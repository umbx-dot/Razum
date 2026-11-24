#include "analysis/analysis.h"

#include "common/arena.h"

static bool scalar_type(ValueType type) {
    return is_integer(type) || type == TYPE_DOUBLE || type == TYPE_BOOL;
}

static Expression *returned_value(const Statement *statement) {
    if (statement == NULL) {
        return NULL;
    }
    if (statement->kind == STATEMENT_BLOCK && statement->block.count == 1) {
        statement = LIST_AT(statement->block, Statement, 0);
    }
    return statement->kind == STATEMENT_RETURN ? statement->return_value : NULL;
}

static const Expression *function_value(const Function *function, Expression *choice) {
    const Statement *body = function->body;
    if (body->kind != STATEMENT_BLOCK || body->block.count < 1 || body->block.count > 2) {
        return NULL;
    }
    const Statement *first = LIST_AT(body->block, Statement, 0);
    if (body->block.count == 1 && first->kind == STATEMENT_RETURN) {
        return first->return_value;
    }
    if (first->kind != STATEMENT_IF || (first->if_statement.else_branch != NULL && body->block.count != 1)) {
        return NULL;
    }
    Expression *when_true = returned_value(first->if_statement.then_branch);
    const Statement *otherwise = body->block.count == 2 ? LIST_AT(body->block, Statement, 1)
                                                       : first->if_statement.else_branch;
    Expression *when_false = returned_value(otherwise);
    if (when_true == NULL || when_false == NULL || when_true->type != function->return_type ||
        when_false->type != function->return_type) {
        return NULL;
    }
    choice->kind = EXPRESSION_CONDITIONAL;
    choice->type = function->return_type;
    choice->shape = function->return_shape;
    choice->conditional.condition = first->if_statement.condition;
    choice->conditional.when_true = when_true;
    choice->conditional.when_false = when_false;
    return choice;
}

static bool capture_free_argument(const Expression *argument, ValueType parameter) {
    if (!scalar_type(parameter)) {
        return false;
    }
    if (argument->type != parameter &&
        !(is_integer(argument->type) && (parameter == TYPE_LONG || parameter == TYPE_ULONG))) {
        return false;
    }
    return argument->kind == EXPRESSION_INT || argument->kind == EXPRESSION_DOUBLE ||
           (argument->kind == EXPRESSION_NAME && argument->name.symbol->kind != SYMBOL_GLOBAL &&
            !argument->name.symbol->address_taken && !argument->name.symbol->is_reference);
}

static Expression *substitute(const Expression *value, Expression *const *arguments, int *budget) {
    if (*budget <= 0 || !scalar_type(value->type)) {
        return NULL;
    }
    --*budget;
    Expression result = *value;
    switch (value->kind) {
        case EXPRESSION_NAME:
            return value->name.symbol->kind == SYMBOL_PARAMETER ? arguments[value->name.symbol->index] : NULL;
        case EXPRESSION_INT:
        case EXPRESSION_DOUBLE:
            break;
        case EXPRESSION_UNARY:
            result.unary.operand = substitute(value->unary.operand, arguments, budget);
            if (result.unary.operand == NULL) {
                return NULL;
            }
            break;
        case EXPRESSION_CAST:
            result.cast.operand = substitute(value->cast.operand, arguments, budget);
            if (result.cast.operand == NULL) {
                return NULL;
            }
            break;
        case EXPRESSION_BINARY:
            result.binary.left = substitute(value->binary.left, arguments, budget);
            result.binary.right = substitute(value->binary.right, arguments, budget);
            if (result.binary.left == NULL || result.binary.right == NULL) {
                return NULL;
            }
            break;
        case EXPRESSION_CONDITIONAL:
            result.conditional.condition = substitute(value->conditional.condition, arguments, budget);
            result.conditional.when_true = substitute(value->conditional.when_true, arguments, budget);
            result.conditional.when_false = substitute(value->conditional.when_false, arguments, budget);
            if (result.conditional.condition == NULL || result.conditional.when_true == NULL ||
                result.conditional.when_false == NULL) {
                return NULL;
            }
            break;
        default:
            return NULL;
    }
    Expression *copy = arena_alloc(sizeof *copy);
    *copy = result;
    return copy;
}

void inline_small_call(Expression *call) {
    enum { MAX_PARAMETERS = 4, MAX_NODES = 12 };
    const Function *target = call->call.target;
    ValueType type = call->type;
    if (target == NULL || target->body == NULL || target->extern_dll != NULL ||
        (type != TYPE_LONG && type != TYPE_ULONG && type != TYPE_DOUBLE && type != TYPE_BOOL) ||
        target->parameters.count > MAX_PARAMETERS || target->parameters.count != call->call.arguments.count) {
        return;
    }
    Expression choice = {0};
    const Expression *value = function_value(target, &choice);
    if (value == NULL || value->type != type) {
        return;
    }
    Expression *arguments[MAX_PARAMETERS];
    for (int i = 0; i < target->parameters.count; i++) {
        const Parameter *parameter = LIST_AT(target->parameters, Parameter, i);
        Expression *argument = LIST_AT(call->call.arguments, Expression, i);
        if (!capture_free_argument(argument, parameter->type)) {
            return;
        }
        if (argument->type != parameter->type) {
            Expression *converted = arena_alloc(sizeof *converted);
            *converted = *argument;
            converted->type = parameter->type;
            converted->shape = parameter->shape;
            argument = converted;
        }
        arguments[i] = argument;
    }
    int budget = MAX_NODES;
    Expression *result = substitute(value, arguments, &budget);
    if (result != NULL) {
        int line = call->line;
        int column = call->column;
        *call = *result;
        call->line = line;
        call->column = column;
    }
}
