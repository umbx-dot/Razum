#include "codegen/packed_double_loop.h"

enum { PACKED_LESS = 1, PACKED_LESS_EQUAL = 2 };

typedef struct {
    const Symbol *symbol;
    int value;
} Binding;

static int find_binding(const Binding *bindings, int count, const Symbol *symbol) {
    for (int i = 0; i < count; i++) {
        if (bindings[i].symbol == symbol) {
            return i;
        }
    }
    return -1;
}

static bool same_input(const Expression *first, const Expression *second) {
    if (first == NULL || first->kind != second->kind) {
        return false;
    }
    return first->kind == EXPRESSION_NAME ? first->name.symbol == second->name.symbol
                                         : double_bits(first->double_literal) == double_bits(second->double_literal);
}

static int append(PackedDoubleLoop *loop, PackedDoubleValue value) {
    if (loop->value_count == PACKED_DOUBLE_REGISTERS - 1) {
        return -1;
    }
    loop->values[loop->value_count] = value;
    return loop->value_count++;
}

static int value_of(PackedDoubleLoop *loop, const Binding *bindings, int count, const Expression *expression,
                    int *budget) {
    if (expression == NULL || expression->type != TYPE_DOUBLE || *budget <= 0) {
        return -1;
    }
    --*budget;
    if (expression->kind == EXPRESSION_NAME || expression->kind == EXPRESSION_DOUBLE) {
        if (expression->kind == EXPRESSION_NAME) {
            const Symbol *symbol = expression->name.symbol;
            int bound = find_binding(bindings, count, symbol);
            if (bound >= 0) {
                return bindings[bound].value;
            }
            if (symbol->kind == SYMBOL_GLOBAL || symbol->address_taken || symbol->is_reference) {
                return -1;
            }
        }
        for (int i = loop->variable_count; i < loop->value_count; i++) {
            if (same_input(loop->values[i].input, expression)) {
                return i;
            }
        }
        return append(loop, (PackedDoubleValue){.input = expression});
    }
    if (expression->kind != EXPRESSION_BINARY || expression->binary.operator < OP_ADD ||
        expression->binary.operator > OP_MULTIPLY) {
        return -1;
    }
    int left = value_of(loop, bindings, count, expression->binary.left, budget);
    int right = value_of(loop, bindings, count, expression->binary.right, budget);
    if (left < 0 || right < 0) {
        return -1;
    }
    Operator op = expression->binary.operator;
    for (int i = loop->variable_count; i < loop->value_count; i++) {
        const PackedDoubleValue *value = &loop->values[i];
        if (value->operator == op && value->left == left && value->right == right) {
            return i;
        }
    }
    return append(loop, (PackedDoubleValue){.operator = op, .left = left, .right = right});
}

static bool plan_guard(PackedDoubleLoop *loop, const Binding *bindings, int count, const Expression *condition) {
    if (condition->kind != EXPRESSION_BINARY || condition->binary.operation != TYPE_DOUBLE) {
        return false;
    }
    Operator op = condition->binary.operator;
    if (op < OP_LESS || op > OP_GREATER_EQUAL) {
        return false;
    }
    int budget = 24;
    int left = value_of(loop, bindings, count, condition->binary.left, &budget);
    int right = value_of(loop, bindings, count, condition->binary.right, &budget);
    if (left < 0 || right < 0) {
        return false;
    }
    loop->compare_left = op <= OP_LESS_EQUAL ? left : right;
    loop->compare_right = op <= OP_LESS_EQUAL ? right : left;
    loop->compare_predicate = op == OP_LESS_EQUAL || op == OP_GREATER_EQUAL ? PACKED_LESS_EQUAL : PACKED_LESS;
    loop->comparison = append(loop, (PackedDoubleValue){0});
    return loop->comparison >= 0;
}

bool plan_packed_double_step(PackedDoubleLoop *loop, const Expression *condition, const Statement *body) {
    Binding bindings[PACKED_DOUBLE_REGISTERS];
    int count = loop->variable_count;
    for (int i = 0; i < count; i++) {
        bindings[i] = (Binding){loop->variables[i].symbol, i};
    }
    if (!plan_guard(loop, bindings, count, condition)) {
        return false;
    }
    int budget = 48;
    for (int i = 0; i + 1 < body->block.count; i++) {
        const Statement *statement = LIST_AT(body->block, Statement, i);
        if (statement->kind == STATEMENT_DECLARATION && statement->declaration.type == TYPE_DOUBLE &&
            count < PACKED_DOUBLE_REGISTERS) {
            int value = value_of(loop, bindings, count, statement->declaration.initial, &budget);
            if (value < 0) {
                return false;
            }
            bindings[count++] = (Binding){statement->declaration.symbol, value};
        } else if (statement->kind == STATEMENT_ASSIGNMENT && statement->assignment.operator == OP_ASSIGN &&
                   statement->assignment.target->kind == EXPRESSION_NAME) {
            int binding = find_binding(bindings, count, statement->assignment.target->name.symbol);
            int value = value_of(loop, bindings, count, statement->assignment.value, &budget);
            if (binding < 0 || value < 0) {
                return false;
            }
            bindings[binding].value = value;
        } else {
            return false;
        }
    }
    for (int i = 0; i < loop->variable_count; i++) {
        int next = bindings[i].value;
        if (next < loop->variable_count && next != i && bindings[next].value != next) {
            return false;
        }
        loop->variables[i].next = next;
    }
    return true;
}
