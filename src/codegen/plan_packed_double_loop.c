#include "codegen/packed_double_loop.h"

#include <stdint.h>

static const Symbol *name(const Expression *value) {
    return value != NULL && value->kind == EXPRESSION_NAME ? value->name.symbol : NULL;
}

static bool integer_value(const Expression *value, int64_t minimum) {
    if (value->kind == EXPRESSION_INT) {
        return value->int_literal.value >= minimum && value->int_literal.value <= INT32_MAX && value->type != TYPE_ULONG;
    }
    const Symbol *symbol = name(value);
    return symbol != NULL && symbol->type == TYPE_INT && symbol->register_name != NULL;
}

static bool binary(const Expression *value, Operator operator) {
    return value != NULL && value->kind == EXPRESSION_BINARY && value->binary.operator == operator;
}

static bool increment(const Statement *statement, const Symbol *symbol) {
    return statement != NULL && statement->kind == STATEMENT_INCREMENT && statement->increment.is_increment &&
           name(statement->increment.target) == symbol;
}

static bool same_integer(const Expression *first, const Expression *second) {
    return first->kind == EXPRESSION_INT ? second->kind == EXPRESSION_INT &&
           first->int_literal.value == second->int_literal.value && second->type != TYPE_ULONG
           : name(first) != NULL && name(first) == name(second);
}

static bool initial_value(PackedDoubleLoop *loop, int variables_ready, const Expression *value,
                          int depth, int *budget) {
    if (value == NULL || depth > 6 || *budget <= 0) {
        return false;
    }
    --*budget;
    const Symbol *symbol = name(value);
    if (symbol != NULL) {
        if (symbol == loop->total || symbol->kind == SYMBOL_GLOBAL || symbol->address_taken || symbol->is_reference ||
            (symbol->type != TYPE_DOUBLE && symbol->type != TYPE_INT)) {
            return false;
        }
        for (int i = variables_ready; i < loop->variable_count; i++) {
            if (symbol == loop->variables[i].symbol) {
                return false;
            }
        }
        return true;
    }
    if (value->kind == EXPRESSION_INT || value->kind == EXPRESSION_DOUBLE) {
        return value->type == TYPE_DOUBLE || integer_value(value, INT32_MIN);
    }
    if (value->type != TYPE_DOUBLE || value->kind != EXPRESSION_BINARY ||
        value->binary.operator < OP_ADD || value->binary.operator > OP_DIVIDE) {
        return false;
    }
    int registers = loop->variable_count + depth + 1;
    if (registers > loop->init_register_count) {
        loop->init_register_count = registers;
    }
    return initial_value(loop, variables_ready, value->binary.left, depth + 1, budget) &&
           initial_value(loop, variables_ready, value->binary.right, depth + 1, budget);
}

static const Symbol *counted_total(const Statement *statement, const Symbol *steps, const Expression *limit) {
    if (statement->kind != STATEMENT_IF || statement->if_statement.else_branch != NULL) {
        return NULL;
    }
    const Expression *test = statement->if_statement.condition;
    if (!binary(test, OP_EQUAL) || name(test->binary.left) != steps || !same_integer(limit, test->binary.right)) {
        return NULL;
    }
    const Statement *count = statement->if_statement.then_branch;
    if (count->kind == STATEMENT_BLOCK && count->block.count == 1) {
        count = LIST_AT(count->block, Statement, 0);
    }
    if (count->kind != STATEMENT_INCREMENT || !count->increment.is_increment) {
        return NULL;
    }
    const Symbol *total = name(count->increment.target);
    return total != NULL && total->register_name != NULL &&
           (total->type == TYPE_LONG || total->type == TYPE_INT) ? total : NULL;
}

bool plan_packed_double_loop(PackedDoubleLoop *loop, const Statement *statement) {
    Expression *condition = statement->for_statement.condition;
    const Statement *body = statement->for_statement.body;
    if (!binary(condition, OP_LESS) || body->kind != STATEMENT_BLOCK ||
        body->block.count < 4 || body->block.count > PACKED_DOUBLE_VARIABLES + 3) {
        return false;
    }
    loop->index = name(condition->binary.left);
    loop->bound = condition->binary.right;
    if (loop->index == NULL || loop->index->type != TYPE_INT || loop->index->register_name == NULL ||
        !integer_value(loop->bound, INT32_MIN) || name(loop->bound) == loop->index ||
        !increment(statement->for_statement.step, loop->index)) {
        return false;
    }
    loop->variable_count = body->block.count - 3;
    loop->init_register_count = loop->variable_count;
    loop->value_count = loop->variable_count;
    for (int i = 0; i < loop->variable_count; i++) {
        const Statement *decl = LIST_AT(body->block, Statement, i);
        if (decl->kind != STATEMENT_DECLARATION || decl->declaration.type != TYPE_DOUBLE) {
            return false;
        }
        loop->variables[i] = (PackedDoubleVariable){decl->declaration.symbol, decl->declaration.initial, i};
    }
    const Statement *counter = LIST_AT(body->block, Statement, loop->variable_count);
    const Statement *repeat = LIST_AT(body->block, Statement, loop->variable_count + 1);
    if (counter->kind != STATEMENT_DECLARATION || counter->declaration.type != TYPE_INT ||
        counter->declaration.initial == NULL || counter->declaration.initial->kind != EXPRESSION_INT ||
        counter->declaration.initial->int_literal.value != 0 || repeat->kind != STATEMENT_WHILE ||
        repeat->while_statement.body->kind != STATEMENT_BLOCK) {
        return false;
    }
    const Symbol *steps = counter->declaration.symbol;
    const Expression *test = repeat->while_statement.condition;
    const Statement *updates = repeat->while_statement.body;
    if (!binary(test, OP_AND) || !binary(test->binary.left, OP_LESS) ||
        name(test->binary.left->binary.left) != steps || updates->block.count < 2 ||
        updates->block.count > PACKED_DOUBLE_REGISTERS ||
        !increment(LIST_AT(updates->block, Statement, updates->block.count - 1), steps)) {
        return false;
    }
    loop->limit = test->binary.left->binary.right;
    if (!integer_value(loop->limit, 1) || name(loop->limit) == steps || name(loop->limit) == loop->index) {
        return false;
    }
    loop->total = counted_total(LIST_AT(body->block, Statement, body->block.count - 1), steps, loop->limit);
    if (loop->total == NULL || loop->total == loop->index || loop->total == name(loop->bound) ||
        loop->total == name(loop->limit)) {
        return false;
    }
    for (int i = 0; i < loop->variable_count; i++) {
        int budget = 16;
        if (!initial_value(loop, i, loop->variables[i].initial, 0, &budget)) {
            return false;
        }
    }
    return plan_packed_double_step(loop, test->binary.right, updates);
}
