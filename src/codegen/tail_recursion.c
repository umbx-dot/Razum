#include "codegen/codegen.h"

static bool is_wide_integer(ValueType type) {
    return type == TYPE_LONG || type == TYPE_ULONG;
}

static Expression *self_call(const Function *function, Expression *expression) {
    if (expression != NULL && expression->kind == EXPRESSION_CALL && expression->call.target == function) {
        return expression;
    }
    return NULL;
}

static Expression *tail_call(const Function *function, Expression *expression, Expression **prefix) {
    if (expression != NULL && expression->kind == EXPRESSION_BINARY && expression->binary.operator == OP_ADD &&
        is_wide_integer(expression->binary.operation)) {
        Expression *call = self_call(function, expression->binary.right);
        if (call != NULL) {
            *prefix = expression->binary.left;
            return call;
        }
    }
    return self_call(function, expression);
}

enum { TAIL_DIRECT = 1, TAIL_ADD = 2 };

static int tail_returns(const Function *function, const Statement *statement) {
    if (statement == NULL) {
        return 0;
    }
    int found = 0;
    switch (statement->kind) {
        case STATEMENT_RETURN: {
            Expression *prefix = NULL;
            if (tail_call(function, statement->return_value, &prefix) == NULL) {
                return 0;
            }
            return prefix == NULL ? TAIL_DIRECT : TAIL_ADD;
        }
        case STATEMENT_IF:
            return tail_returns(function, statement->if_statement.then_branch) |
                   tail_returns(function, statement->if_statement.else_branch);
        case STATEMENT_WHILE:
        case STATEMENT_DO:
            return tail_returns(function, statement->while_statement.body);
        case STATEMENT_FOR:
            return tail_returns(function, statement->for_statement.body);
        case STATEMENT_BLOCK:
            for (int index = 0; index < statement->block.count; index++) {
                found |= tail_returns(function, LIST_AT(statement->block, Statement, index));
            }
            return found;
        case STATEMENT_SWITCH:
            for (int index = 0; index < statement->switch_statement.cases.count; index++) {
                SwitchCase *branch = LIST_AT(statement->switch_statement.cases, SwitchCase, index);
                found |= tail_returns(function, branch->body);
            }
            return found;
        default:
            return 0;
    }
}

void setup_tail_recursion(FunctionContext *context) {
    const Function *function = context->function;
    if (!context->program->optimize || function->parameters.count != 1 || !is_wide_integer(function->return_type)) {
        return;
    }
    const Symbol *parameter = LIST_AT(function->parameters, Parameter, 0)->symbol;
    if (!is_wide_integer(parameter->type) || parameter->register_name == NULL) {
        return;
    }
    for (int index = 0; index < function->locals.count; index++) {
        const Symbol *local = LIST_AT(function->locals, Symbol, index);
        if (local->address_taken || is_aggregate(local->type)) {
            return;
        }
    }
    int mode = tail_returns(function, function->body);
    if ((mode & TAIL_ADD) != 0) {
        static const char *const REGISTERS[] = {"rbx", "rsi", "rdi", "r12", "r13", "r14", "r15"};
        if (context->saved_count == (int)(sizeof REGISTERS / sizeof REGISTERS[0])) {
            return;
        }
        context->tail_sum = REGISTERS[context->saved_count];
        context->saved_registers[context->saved_count++] = context->tail_sum;
    }
    if (mode != 0) {
        context->tail_label = new_label(context, "recur");
    }
}

bool emit_tail_return(FunctionContext *context, Expression *expression) {
    Expression *prefix = NULL;
    Expression *call = tail_call(context->function, expression, &prefix);
    if (call == NULL) {
        return false;
    }
    if (prefix != NULL) {
        emit_int(context, prefix);
        emit(context, "add %s, rax", context->tail_sum);
    }
    const Symbol *parameter = LIST_AT(context->function->parameters, Parameter, 0)->symbol;
    Expression *argument = LIST_AT(call->call.arguments, Expression, 0);
    if (!emit_wide_offset(context, argument, parameter->register_name)) {
        emit_int(context, argument);
        emit(context, "mov %s, rax", parameter->register_name);
    }
    emit(context, "jmp %s", context->tail_label);
    return true;
}
