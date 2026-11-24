#include "codegen/codegen.h"

bool double_product_operands(const Expression *expression, const Symbol **left, const Symbol **right) {
    if (expression->kind != EXPRESSION_BINARY || expression->type != TYPE_DOUBLE ||
        expression->binary.operator != OP_MULTIPLY) {
        return false;
    }
    const Expression *a = expression->binary.left;
    const Expression *b = expression->binary.right;
    if (a->kind != EXPRESSION_NAME || a->type != TYPE_DOUBLE || a->name.symbol->register_name == NULL ||
        b->kind != EXPRESSION_NAME || b->type != TYPE_DOUBLE || b->name.symbol->register_name == NULL) {
        return false;
    }
    *left = a->name.symbol;
    *right = b->name.symbol;
    return true;
}

const char *cached_double_product(FunctionContext *context, const Expression *expression) {
    LoopProducts *products = context->loop_products;
    const Symbol *left;
    const Symbol *right;
    if (products == NULL || !double_product_operands(expression, &left, &right)) {
        return NULL;
    }
    for (int index = 0; index < products->count; index++) {
        LoopProduct *product = &products->entries[index];
        if (left != product->left || right != product->right ||
            (!products->testing && products->statement > product->last_statement)) {
            continue;
        }
        if (products->testing && !product->ready) {
            if (context->program->zen3) {
                emit(context, "vmulsd %s, %s, %s", product->reg, left->register_name, right->register_name);
            } else {
                emit(context, "movapd %s, %s", product->reg, left->register_name);
                emit(context, "mulsd %s, %s", product->reg, right->register_name);
            }
            product->ready = true;
        }
        return product->reg;
    }
    return NULL;
}
