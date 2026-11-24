#include "codegen/codegen.h"

static bool conjunction(const Expression *expression) {
    return expression->kind == EXPRESSION_BINARY && expression->binary.operator == OP_AND;
}

static bool contains_product(const Expression *expression, const Symbol *left, const Symbol *right) {
    if (expression->kind != EXPRESSION_BINARY) {
        return false;
    }
    const Symbol *a;
    const Symbol *b;
    return (double_product_operands(expression, &a, &b) && a == left && b == right) ||
           contains_product(expression->binary.left, left, right) ||
           contains_product(expression->binary.right, left, right);
}

static const Expression *double_assignment(const Statement *statement, const Symbol **target) {
    const Expression *value;
    if (statement->kind == STATEMENT_DECLARATION) {
        *target = statement->declaration.symbol;
        value = statement->declaration.initial;
    } else if (statement->kind == STATEMENT_ASSIGNMENT && statement->assignment.operator == OP_ASSIGN &&
               statement->assignment.target->kind == EXPRESSION_NAME) {
        *target = statement->assignment.target->name.symbol;
        value = statement->assignment.value;
    } else {
        return NULL;
    }
    return (*target)->type == TYPE_DOUBLE && (*target)->register_name != NULL && value != NULL &&
           double_tree_fits(value) ? value : NULL;
}

static int last_use(const Statement *body, const Symbol *left, const Symbol *right) {
    int last = -1;
    for (int i = 0; i < body->block.count; i++) {
        const Statement *statement = LIST_AT(body->block, Statement, i);
        const Symbol *target;
        const Expression *value = double_assignment(statement, &target);
        if (value == NULL) {
            break;
        }
        if (contains_product(value, left, right)) {
            last = i;
        }
        if (target == left || target == right) {
            break;
        }
    }
    return last;
}

static void collect(FunctionContext *context, const Expression *expression, const Statement *body, LoopProducts *products) {
    if (expression->kind != EXPRESSION_BINARY || products->count == (int)(sizeof products->entries / sizeof products->entries[0])) {
        return;
    }
    const Symbol *left;
    const Symbol *right;
    if (!double_product_operands(expression, &left, &right)) {
        collect(context, expression->binary.left, body, products);
        collect(context, expression->binary.right, body, products);
        return;
    }
    for (int i = 0; i < products->count; i++) {
        if (left == products->entries[i].left && right == products->entries[i].right) {
            return;
        }
    }
    int last = last_use(body, left, right);
    if (last >= 0) {
        const char *reg = reserve_loop_vector(context, products->count);
        if (reg != NULL) {
            products->entries[products->count++] = (LoopProduct){left, right, reg, last, false};
        }
    }
}

void plan_loop_products(FunctionContext *context, Expression *condition, const Statement *body, LoopProducts *products) {
    if (body->kind != STATEMENT_BLOCK || condition == NULL) {
        return;
    }
    while (conjunction(condition)) {
        condition = condition->binary.right;
    }
    if (condition->kind != EXPRESSION_BINARY || condition->binary.operation != TYPE_DOUBLE ||
        condition->binary.operator < OP_EQUAL || condition->binary.operator > OP_GREATER_EQUAL ||
        !double_tree_fits(condition->binary.left) || !double_tree_fits(condition->binary.right)) {
        return;
    }
    collect(context, condition->binary.left, body, products);
    collect(context, condition->binary.right, body, products);
}

void emit_cached_loop_test(FunctionContext *context, Expression *condition, const char *top, const char *end, LoopProducts *products) {
    if (products->count == 0) {
        jump_if(context, condition, top, true);
        return;
    }
    while (conjunction(condition)) {
        jump_if(context, condition->binary.left, end, false);
        condition = condition->binary.right;
    }
    products->testing = true;
    context->loop_products = products;
    jump_if(context, condition, top, true);
    context->loop_products = NULL;
}
