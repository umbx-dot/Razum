#include "codegen/vector_loop.h"

#include <stdint.h>

static bool lane_type(ValueType type) {
    return type == TYPE_INT || type == TYPE_UINT || type == TYPE_LONG || type == TYPE_ULONG;
}

static const Symbol *name_symbol(const Expression *expression) {
    return expression->kind == EXPRESSION_NAME ? expression->name.symbol : NULL;
}

static const Symbol *indexed_base(const Expression *value, const Symbol *index) {
    if (value->kind != EXPRESSION_INDEX || name_symbol(value->index.index) != index) {
        return NULL;
    }
    const Symbol *base = name_symbol(value->index.base);
    return base != NULL && (base->type == TYPE_ARRAY || (base->type == TYPE_POINTER && base->register_name != NULL))
           ? base : NULL;
}

static bool fits_bound(const Expression *bound, const Symbol *index) {
    if (bound->kind == EXPRESSION_INT) {
        int64_t value = bound->int_literal.value;
        switch (index->type) {
            case TYPE_INT: return value >= INT32_MIN && value <= INT32_MAX;
            case TYPE_UINT: return value >= 0 && value <= UINT32_MAX;
            case TYPE_LONG: return bound->type != TYPE_ULONG;
            default: return value >= 0 || bound->type == TYPE_ULONG;
        }
    }
    const Symbol *symbol = name_symbol(bound);
    return symbol != NULL && symbol->register_name != NULL && symbol->type == index->type;
}

static bool fits(VectorLoop *loop, const Expression *value, int depth) {
    if (value->kind == EXPRESSION_INT) {
        return true;
    }
    if (!lane_type(value->type) || type_info(value->type)->size != loop->width) {
        return false;
    }
    const Symbol *symbol = name_symbol(value);
    if (symbol != NULL) {
        loop->uses_index |= symbol == loop->index;
        return symbol != loop->sum && symbol->register_name != NULL;
    }
    if (value->kind == EXPRESSION_INDEX) {
        const Symbol *base = indexed_base(value, loop->index);
        const Symbol *store = loop->store_base;
        return base != NULL && (store == NULL || base == store ||
               (base->type == TYPE_ARRAY && !base->is_reference && store->type == TYPE_ARRAY && !store->is_reference));
    }
    if (value->kind != EXPRESSION_BINARY) {
        return false;
    }
    const Expression *left = value->binary.left;
    const Expression *right = value->binary.right;
    Operator op = value->binary.operator;
    if (is_shift(op)) {
        return right->kind == EXPRESSION_INT && fits(loop, left, depth);
    }
    if (op == OP_MULTIPLY && loop->width == 8) {
        loop->uses_index = true;
        return name_symbol(left) == loop->index && name_symbol(right) == loop->index;
    }
    return depth < 2 && integer_arithmetic_instruction(op) != NULL &&
           fits(loop, left, depth) && fits(loop, right, depth + 1);
}

static const Expression *loop_value(VectorLoop *loop, const Statement *body) {
    if (body->kind == STATEMENT_BLOCK && body->block.count == 1) {
        body = LIST_AT(body->block, Statement, 0);
    }
    if (body->kind != STATEMENT_ASSIGNMENT) {
        return NULL;
    }
    const Expression *target = body->assignment.target;
    if (!lane_type(target->type)) {
        return NULL;
    }
    loop->width = type_info(target->type)->size;
    const Expression *value = body->assignment.value;
    const Symbol *sum = name_symbol(target);
    if (sum == NULL) {
        loop->store_base = indexed_base(target, loop->index);
        return loop->store_base != NULL && body->assignment.operator == OP_ASSIGN ? value : NULL;
    }
    if (sum == loop->index || sum == name_symbol(loop->bound) || sum->register_name == NULL) {
        return NULL;
    }
    loop->sum = sum;
    if (body->assignment.operator == OP_ADD) {
        return value;
    }
    if (body->assignment.operator == OP_ASSIGN && value->kind == EXPRESSION_BINARY &&
        value->binary.operator == OP_ADD && name_symbol(value->binary.left) == sum) {
        return value->binary.right;
    }
    return NULL;
}

static const Expression *initializer(const Statement *statement, const Symbol *index) {
    if (statement != NULL && statement->kind == STATEMENT_DECLARATION && statement->declaration.symbol == index) {
        return statement->declaration.initial;
    }
    if (statement != NULL && statement->kind == STATEMENT_ASSIGNMENT && statement->assignment.operator == OP_ASSIGN &&
        name_symbol(statement->assignment.target) == index) {
        return statement->assignment.value;
    }
    return NULL;
}

bool plan_vector_loop(VectorLoop *loop, const Statement *statement, bool avx) {
    const Statement *step = statement->for_statement.step;
    const Expression *condition = statement->for_statement.condition;
    if (step == NULL || step->kind != STATEMENT_INCREMENT || !step->increment.is_increment || condition == NULL ||
        condition->kind != EXPRESSION_BINARY || condition->binary.operator != OP_LESS) {
        return false;
    }
    const Symbol *index = name_symbol(step->increment.target);
    Expression *bound = condition->binary.right;
    if (index == NULL || !lane_type(index->type) || index->register_name == NULL ||
        name_symbol(condition->binary.left) != index || !fits_bound(bound, index) ||
        (condition->binary.operation == TYPE_ULONG) != (index->type == TYPE_ULONG)) {
        return false;
    }
    const Expression *initial = initializer(statement->for_statement.initial, index);
    loop->index = index;
    loop->bound = bound;
    loop->value = loop_value(loop, statement->for_statement.body);
    if (loop->value == NULL) {
        return false;
    }
    loop->comparison = condition->binary.operation;
    loop->lanes = (avx ? 32 : 16) / loop->width;
    loop->avx = avx;
    loop->nonnegative_index = initial != NULL && initial->kind == EXPRESSION_INT &&
                              initial->int_literal.value >= 0 && fits_bound(initial, index);
    loop->index_fits_u32 = loop->nonnegative_index && bound->kind == EXPRESSION_INT &&
                           bound->int_literal.value >= 0 && bound->int_literal.value <= UINT32_MAX;
    return fits(loop, loop->value, 0);
}
