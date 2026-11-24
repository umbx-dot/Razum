#include "codegen/codegen.h"

typedef struct {
    const Function *function;
    const Symbol *parameter;
    const Expression *base;
    const uint64_t *table;
    int first;
    int input;
    int calls;
} Recurrence;

static bool is_parameter(const Expression *expression, const Symbol *parameter) {
    return expression->kind == EXPRESSION_NAME && expression->name.symbol == parameter;
}

static uint64_t base_value(const Expression *base, int64_t input) {
    return base->kind == EXPRESSION_INT ? (uint64_t)base->int_literal.value : (uint64_t)input;
}

static bool evaluate(Recurrence *state, const Expression *expression, uint64_t *result) {
    if (expression->kind == EXPRESSION_INT || is_parameter(expression, state->parameter)) {
        *result = base_value(expression, state->input);
        return true;
    }
    if (expression->kind == EXPRESSION_CALL && expression->call.target == state->function) {
        const Expression *argument = LIST_AT(expression->call.arguments, Expression, 0);
        if (argument->kind != EXPRESSION_BINARY || argument->binary.operator != OP_SUBTRACT ||
            argument->type != TYPE_LONG || !is_parameter(argument->binary.left, state->parameter) ||
            argument->binary.right->kind != EXPRESSION_INT) {
            return false;
        }
        int64_t step = argument->binary.right->int_literal.value;
        if (step <= 0 || step > 64) {
            return false;
        }
        int input = state->input - (int)step;
        *result = input < state->first ? base_value(state->base, input) : state->table[input - state->first];
        state->calls++;
        return true;
    }
    if (expression->kind != EXPRESSION_BINARY ||
        (expression->binary.operation != TYPE_LONG && expression->binary.operation != TYPE_ULONG)) {
        return false;
    }
    uint64_t left;
    uint64_t right;
    if (!evaluate(state, expression->binary.left, &left) || !evaluate(state, expression->binary.right, &right)) {
        return false;
    }
    switch (expression->binary.operator) {
        case OP_ADD: *result = left + right; return true;
        case OP_SUBTRACT: *result = left - right; return true;
        case OP_MULTIPLY: *result = left * right; return true;
        case OP_BIT_AND: *result = left & right; return true;
        case OP_BIT_OR: *result = left | right; return true;
        case OP_BIT_XOR: *result = left ^ right; return true;
        default: return false;
    }
}

bool small_recurrence(const Function *function, const Statement *guard, uint64_t *table, int size, int *first) {
    const Statement *body = function->body;
    const Symbol *parameter = LIST_AT(function->parameters, Parameter, 0)->symbol;
    if (function->return_type != TYPE_LONG || parameter->type != TYPE_LONG ||
        body->kind != STATEMENT_BLOCK || body->block.count != 2) {
        return false;
    }
    const Expression *condition = guard->if_statement.condition;
    if (condition->binary.operation != TYPE_LONG || condition->binary.operator != OP_LESS ||
        condition->binary.right->int_literal.value < 0 || condition->binary.right->int_literal.value > 16) {
        return false;
    }
    const Statement *base = guard->if_statement.then_branch;
    if (base->kind == STATEMENT_BLOCK) {
        base = LIST_AT(base->block, Statement, 0);
    }
    const Statement *step = LIST_AT(body->block, Statement, 1);
    if (step->kind != STATEMENT_RETURN || step->return_value == NULL) {
        return false;
    }
    *first = (int)condition->binary.right->int_literal.value;
    Recurrence recurrence = {function, parameter, base->return_value, table, *first, 0, 0};
    for (int index = 0; index < size; index++) {
        recurrence.input = *first + index;
        recurrence.calls = 0;
        if (!evaluate(&recurrence, step->return_value, &table[index]) || recurrence.calls < 2) {
            return false;
        }
    }
    return true;
}
