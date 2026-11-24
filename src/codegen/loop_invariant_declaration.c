#include "codegen/codegen.h"

static bool writes_symbol(const Statement *statement, const Symbol *symbol) {
    if (statement == NULL) {
        return false;
    }
    switch (statement->kind) {
        case STATEMENT_BLOCK:
            for (int i = 0; i < statement->block.count; i++) {
                if (writes_symbol(LIST_AT(statement->block, Statement, i), symbol)) {
                    return true;
                }
            }
            return false;
        case STATEMENT_DECLARATION:
            return statement->declaration.symbol == symbol;
        case STATEMENT_ASSIGNMENT: {
            const Expression *target = statement->assignment.target;
            return target->kind == EXPRESSION_NAME && target->name.symbol == symbol;
        }
        case STATEMENT_INCREMENT: {
            const Expression *target = statement->increment.target;
            return target->kind == EXPRESSION_NAME && target->name.symbol == symbol;
        }
        case STATEMENT_IF:
            return writes_symbol(statement->if_statement.then_branch, symbol) ||
                   writes_symbol(statement->if_statement.else_branch, symbol);
        case STATEMENT_FOR:
            return writes_symbol(statement->for_statement.initial, symbol) ||
                   writes_symbol(statement->for_statement.step, symbol) ||
                   writes_symbol(statement->for_statement.body, symbol);
        case STATEMENT_WHILE:
        case STATEMENT_DO:
            return writes_symbol(statement->while_statement.body, symbol);
        case STATEMENT_SWITCH:
            for (int i = 0; i < statement->switch_statement.cases.count; i++) {
                if (writes_symbol(LIST_AT(statement->switch_statement.cases, SwitchCase, i)->body, symbol)) {
                    return true;
                }
            }
            return false;
        case STATEMENT_ASM:
            return true;
        default:
            return false;
    }
}

static bool stable_symbol(const Symbol *symbol, const Statement *body, const Statement *step) {
    return symbol->kind != SYMBOL_GLOBAL && !symbol->address_taken && !symbol->is_reference &&
           !writes_symbol(body, symbol) && !writes_symbol(step, symbol);
}

static int invariant_cost(const Expression *value, const Statement *body, const Statement *step, int *budget) {
    if (*budget <= 0) {
        return -1;
    }
    --*budget;
    switch (value->kind) {
        case EXPRESSION_NAME:
            return (is_integer(value->type) || value->type == TYPE_POINTER) &&
                   stable_symbol(value->name.symbol, body, step) ? 0 : -1;
        case EXPRESSION_INT:
            return is_integer(value->type) ? 0 : -1;
        case EXPRESSION_CAST:
            return is_integer(value->type) && is_integer(value->cast.operand->type)
                   ? invariant_cost(value->cast.operand, body, step, budget) : -1;
        case EXPRESSION_UNARY: {
            if (!is_integer(value->type)) {
                return -1;
            }
            int cost = invariant_cost(value->unary.operand, body, step, budget);
            return cost < 0 ? -1 : cost + 1;
        }
        case EXPRESSION_BINARY: {
            if (!is_integer(value->type)) {
                return -1;
            }
            int left = invariant_cost(value->binary.left, body, step, budget);
            int right = invariant_cost(value->binary.right, body, step, budget);
            Operator op = value->binary.operator;
            int cost = op == OP_DIVIDE || op == OP_REMAINDER ? 2 : 1;
            return left < 0 || right < 0 ? -1 : left + right + cost;
        }
        case EXPRESSION_ADDRESS: {
            const Expression *place = value->unary.operand;
            if (place->kind != EXPRESSION_INDEX || place->index.base->kind != EXPRESSION_NAME ||
                place->index.base->type != TYPE_POINTER) {
                return -1;
            }
            int base = invariant_cost(place->index.base, body, step, budget);
            int index = invariant_cost(place->index.index, body, step, budget);
            return base < 0 || index < 0 ? -1 : index + 2;
        }
        default:
            return -1;
    }
}

Statement *loop_invariant_declaration(const Statement *body, const Statement *step) {
    if (body->kind != STATEMENT_BLOCK || body->block.count < 2) {
        return NULL;
    }
    Statement *first = LIST_AT(body->block, Statement, 0);
    if (first->kind != STATEMENT_DECLARATION || first->declaration.initial == NULL) {
        return NULL;
    }
    const Symbol *target = first->declaration.symbol;
    if (target->register_name == NULL || !(is_integer(target->type) || target->type == TYPE_POINTER)) {
        return NULL;
    }
    for (int i = 1; i < body->block.count; i++) {
        if (writes_symbol(LIST_AT(body->block, Statement, i), target)) {
            return NULL;
        }
    }
    int budget = 12;
    return invariant_cost(first->declaration.initial, body, step, &budget) >= 2 ? first : NULL;
}
