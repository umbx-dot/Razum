#include "analysis/analysis.h"

static void mark_function(Function *function, bool optimize);
static void mark_expression(Expression *expression, bool optimize, unsigned weight);

static void mark_call(const Expression *call, bool optimize, unsigned weight) {
    mark_expression(call->call.callee, optimize, weight);
    for (int index = 0; index < call->call.arguments.count; index++) {
        mark_expression(LIST_AT(call->call.arguments, Expression, index), optimize, weight);
    }
    if (call->call.target != NULL) {
        mark_function(call->call.target, optimize);
    }
}

static void mark_expression(Expression *expression, bool optimize, unsigned weight) {
    if (expression == NULL) {
        return;
    }
    switch (expression->kind) {
        case EXPRESSION_FUNCTION:
            mark_function(expression->function_value, optimize);
            return;
        case EXPRESSION_CALL:
            mark_call(expression, optimize, weight);
            if (optimize) {
                inline_small_call(expression);
            }
            return;
        case EXPRESSION_UNARY:
            mark_expression(expression->unary.operand, optimize, weight);
            return;
        case EXPRESSION_BINARY:
            mark_expression(expression->binary.left, optimize, weight);
            mark_expression(expression->binary.right, optimize, weight);
            return;
        case EXPRESSION_CAST:
            mark_expression(expression->cast.operand, optimize, weight);
            return;
        case EXPRESSION_NAME:
            if (expression->name.symbol != NULL) {
                Symbol *symbol = expression->name.symbol;
                symbol->is_used = true;
                if (symbol->kind != SYMBOL_GLOBAL) {
                    symbol->loop_uses += weight;
                }
            }
            return;
        case EXPRESSION_INDEX: {
            Expression *base = expression->index.base;
            Expression *index = expression->index.index;
            const Symbol *base_symbol = base->kind == EXPRESSION_NAME ? base->name.symbol : NULL;
            const Symbol *index_symbol = index->kind == EXPRESSION_NAME ? index->name.symbol : NULL;
            int scale = shape_size(expression->shape);
            if (base_symbol != NULL && index_symbol != NULL &&
                (base->type == TYPE_POINTER || base->type == TYPE_STRING) &&
                base_symbol->kind != SYMBOL_GLOBAL && index_symbol->kind != SYMBOL_GLOBAL &&
                !base_symbol->address_taken && !index_symbol->address_taken &&
                (scale == 1 || scale == 2 || scale == 4 || scale == 8)) {
                weight *= 4;
            }
            mark_expression(base, optimize, weight);
            mark_expression(index, optimize, weight);
            return;
        }
        case EXPRESSION_FIELD:
            mark_expression(expression->field.base, optimize, weight);
            return;
        case EXPRESSION_ADDRESS:
        case EXPRESSION_DEREF:
            mark_expression(expression->unary.operand, optimize, weight);
            return;
        case EXPRESSION_NEW:
            mark_expression(expression->allocation.count, optimize, weight);
            return;
        case EXPRESSION_CONDITIONAL:
            mark_expression(expression->conditional.condition, optimize, weight);
            mark_expression(expression->conditional.when_true, optimize, weight);
            mark_expression(expression->conditional.when_false, optimize, weight);
            return;
        case EXPRESSION_LIST:
            for (int index = 0; index < expression->list.count; index++) {
                mark_expression(LIST_AT(expression->list, Expression, index), optimize, weight);
            }
            return;
        default:
            return;
    }
}

static unsigned loop_use_weight(unsigned weight) {
    if (weight == 0) {
        return 1;
    }
    return weight < 64 ? weight * 4 : weight;
}

static void mark_statement(const Statement *statement, bool optimize, unsigned weight) {
    if (statement == NULL) {
        return;
    }
    switch (statement->kind) {
        case STATEMENT_BLOCK:
            for (int index = 0; index < statement->block.count; index++) {
                mark_statement(LIST_AT(statement->block, Statement, index), optimize, weight);
            }
            return;
        case STATEMENT_DECLARATION:
            mark_expression(statement->declaration.initial, optimize, weight);
            return;
        case STATEMENT_ASSIGNMENT:
            mark_expression(statement->assignment.target, optimize, weight);
            mark_expression(statement->assignment.value, optimize, weight);
            return;
        case STATEMENT_INCREMENT:
            mark_expression(statement->increment.target, optimize, weight);
            return;
        case STATEMENT_IF:
            mark_expression(statement->if_statement.condition, optimize, weight);
            mark_statement(statement->if_statement.then_branch, optimize, weight);
            mark_statement(statement->if_statement.else_branch, optimize, weight);
            return;
        case STATEMENT_WHILE:
        case STATEMENT_DO:
            weight = loop_use_weight(weight);
            mark_expression(statement->while_statement.condition, optimize, weight);
            mark_statement(statement->while_statement.body, optimize, weight);
            return;
        case STATEMENT_SWITCH:
            mark_expression(statement->switch_statement.value, optimize, weight);
            for (int index = 0; index < statement->switch_statement.cases.count; index++) {
                mark_statement(LIST_AT(statement->switch_statement.cases, SwitchCase, index)->body, optimize, weight);
            }
            return;
        case STATEMENT_FOR:
            mark_statement(statement->for_statement.initial, optimize, weight);
            weight = loop_use_weight(weight);
            mark_expression(statement->for_statement.condition, optimize, weight);
            mark_statement(statement->for_statement.step, optimize, weight);
            mark_statement(statement->for_statement.body, optimize, weight);
            return;
        case STATEMENT_RETURN:
            mark_expression(statement->return_value, optimize, weight);
            return;
        case STATEMENT_CALL:
            mark_call(statement->call, optimize, weight);
            return;
        default:
            return;
    }
}

static void mark_function(Function *function, bool optimize) {
    if (function->is_used) {
        return;
    }
    function->is_used = true;
    mark_statement(function->body, optimize, 0);
}

void mark_used_functions(Program *program) {
    for (int index = 0; index < program->globals.count; index++) {
        mark_expression(LIST_AT(program->globals, GlobalVariable, index)->initial, false, 0);
    }
    for (int index = 0; index < program->functions.count; index++) {
        Function *function = LIST_AT(program->functions, Function, index);
        if (!function->is_library || function->is_exported) {
            mark_function(function, program->optimize);
        }
    }
}
