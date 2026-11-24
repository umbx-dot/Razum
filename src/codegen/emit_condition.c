#include "codegen/codegen.h"

#include <string.h>

typedef struct {
    const char *registers;
    const char *when_true;
    const char *when_false;
} DoubleOrder;

static bool is_comparison(Operator operator) {
    return operator >= OP_EQUAL && operator <= OP_GREATER_EQUAL;
}

static const char *int_condition(Operator operator, bool when_true, bool is_unsigned) {
    static const char *const UNSIGNED_TRUE_CODES[] = {
        [OP_EQUAL] = "e", [OP_NOT_EQUAL] = "ne", [OP_LESS] = "b",
        [OP_LESS_EQUAL] = "be", [OP_GREATER] = "a", [OP_GREATER_EQUAL] = "ae",
    };
    static const char *const UNSIGNED_FALSE_CODES[] = {
        [OP_EQUAL] = "ne", [OP_NOT_EQUAL] = "e", [OP_LESS] = "ae",
        [OP_LESS_EQUAL] = "a", [OP_GREATER] = "be", [OP_GREATER_EQUAL] = "b",
    };
    static const char *const TRUE_CODES[] = {
        [OP_EQUAL] = "e", [OP_NOT_EQUAL] = "ne", [OP_LESS] = "l",
        [OP_LESS_EQUAL] = "le", [OP_GREATER] = "g", [OP_GREATER_EQUAL] = "ge",
    };
    static const char *const FALSE_CODES[] = {
        [OP_EQUAL] = "ne", [OP_NOT_EQUAL] = "e", [OP_LESS] = "ge",
        [OP_LESS_EQUAL] = "g", [OP_GREATER] = "le", [OP_GREATER_EQUAL] = "l",
    };
    if (is_unsigned) {
        return when_true ? UNSIGNED_TRUE_CODES[operator] : UNSIGNED_FALSE_CODES[operator];
    }
    return when_true ? TRUE_CODES[operator] : FALSE_CODES[operator];
}

static bool select_operand(const Expression *expression, Operand *operand) {
    if (expression->kind == EXPRESSION_NAME) {
        const Symbol *symbol = expression->name.symbol;
        if (symbol->kind == SYMBOL_GLOBAL || symbol->address_taken || symbol->is_reference) {
            return false;
        }
    }
    return int_operand(expression, operand);
}

bool emit_conditional_move(FunctionContext *context, const Expression *expression) {
    const Expression *condition = expression->conditional.condition;
    Operand left, right, when_true, when_false;
    if (condition->kind != EXPRESSION_BINARY || !is_comparison(condition->binary.operator) ||
        is_floating(condition->binary.operation) ||
        !select_operand(condition->binary.left, &left) || !select_operand(condition->binary.right, &right) ||
        !select_operand(expression->conditional.when_true, &when_true) ||
        !select_operand(expression->conditional.when_false, &when_false)) {
        return false;
    }
    bool use_rax = is_immediate(&left) || (left.text[0] == 'q' && right.text[0] == 'q');
    if (use_rax) {
        emit(context, "mov rax, %s", left.text);
    }
    emit(context, "cmp %s, %s", use_rax ? "rax" : left.text, right.text);
    if (!use_rax || strcmp(left.text, when_false.text) != 0) {
        emit(context, "mov rax, %s", when_false.text);
    }
    const char *source = when_true.text;
    if (is_immediate(&when_true)) {
        emit(context, "mov rcx, %s", source);
        source = "rcx";
    }
    const char *code = int_condition(condition->binary.operator, true, condition->binary.operation == TYPE_ULONG);
    emit(context, "cmov%s rax, %s", code, source);
    return true;
}

static DoubleOrder double_order(Operator operator) {
    switch (operator) {
        case OP_LESS: return (DoubleOrder){"xmm1, xmm0", "a", "be"};
        case OP_LESS_EQUAL: return (DoubleOrder){"xmm1, xmm0", "ae", "b"};
        case OP_GREATER: return (DoubleOrder){"xmm0, xmm1", "a", "be"};
        default: return (DoubleOrder){"xmm0, xmm1", "ae", "b"};
    }
}

static void jump_on_equal(FunctionContext *context, bool jump_when_equal, const char *target) {
    if (jump_when_equal) {
        const char *skip = new_label(context, "skip");
        emit(context, "jp %s", skip);
        emit(context, "je %s", target);
        emit_label(context, skip);
    } else {
        emit(context, "jp %s", target);
        emit(context, "jne %s", target);
    }
}

static void jump_if_double_comparison(FunctionContext *context, Expression *expression, const char *target, bool when_true) {
    Operand operand = emit_double_pair(context, expression->binary.left, expression->binary.right);
    if (strcmp(operand.text, "xmm1") != 0) {
        emit(context, "movsd xmm1, %s", operand.text);
    }
    Operator operator = expression->binary.operator;
    if (operator == OP_EQUAL || operator == OP_NOT_EQUAL) {
        emit(context, "ucomisd xmm0, xmm1");
        jump_on_equal(context, when_true ? operator == OP_EQUAL : operator == OP_NOT_EQUAL, target);
        return;
    }
    DoubleOrder order = double_order(operator);
    emit(context, "ucomisd %s", order.registers);
    emit(context, "j%s %s", when_true ? order.when_true : order.when_false, target);
}

static void jump_if_comparison(FunctionContext *context, Expression *expression, const char *target, bool when_true) {
    ValueType operation = expression->binary.operation;
    if (!is_floating(operation)) {
        Expression *left = expression->binary.left;
        Expression *right = expression->binary.right;
        Operand operand;
        Operator op = expression->binary.operator;
        const char *value;
        if (context->program->optimize && mask_test_operands(expression, &value, &operand)) {
            emit(context, "test %s, %s", value, operand.text);
            emit(context, "j%s %s", int_condition(op, when_true, operation == TYPE_ULONG), target);
            return;
        }
        char place[OPERAND_SIZE];
        if (context->program->optimize && left->kind == EXPRESSION_INDEX && int_operand(right, &operand) &&
            operand.text[0] != 'q' && place_text(left, place, sizeof place)) {
            if (left->type == TYPE_LONG || left->type == TYPE_ULONG) {
                emit(context, "cmp qword [%s], %s", place, operand.text);
                emit(context, "j%s %s", int_condition(op, when_true, operation == TYPE_ULONG), target);
                return;
            }
            if ((op == OP_EQUAL || op == OP_NOT_EQUAL) && right->kind == EXPRESSION_INT &&
                right->int_literal.value == 0 && (left->type == TYPE_BYTE || left->type == TYPE_SBYTE || left->type == TYPE_CHAR)) {
                emit(context, "cmp byte [%s], 0", place);
                emit(context, "j%s %s", int_condition(op, when_true, false), target);
                return;
            }
        }
        if (context->program->optimize && left->kind == EXPRESSION_NAME &&
            left->name.symbol->register_name != NULL && int_operand(right, &operand)) {
            emit(context, "cmp %s, %s", left->name.symbol->register_name, operand.text);
        } else {
            operand = emit_int_pair(context, left, right);
            emit(context, "cmp rax, %s", operand.text);
        }
        emit(context, "j%s %s", int_condition(expression->binary.operator, when_true, operation == TYPE_ULONG), target);
        return;
    }
    jump_if_double_comparison(context, expression, target, when_true);
}

static void jump_if_logical(FunctionContext *context, Expression *expression, const char *target, bool when_true) {
    bool is_and = expression->binary.operator == OP_AND;
    if (is_and != when_true) {
        jump_if(context, expression->binary.left, target, when_true);
        jump_if(context, expression->binary.right, target, when_true);
        return;
    }
    const char *skip = new_label(context, "skip");
    jump_if(context, expression->binary.left, skip, !when_true);
    jump_if(context, expression->binary.right, target, when_true);
    emit_label(context, skip);
}

void jump_if(FunctionContext *context, Expression *expression, const char *target, bool when_true) {
    if (context->program->optimize && expression->kind == EXPRESSION_INT) {
        if ((expression->int_literal.value != 0) == when_true) {
            emit(context, "jmp %s", target);
        }
        return;
    }
    if (expression->kind == EXPRESSION_UNARY && expression->unary.operator == OP_NOT) {
        jump_if(context, expression->unary.operand, target, !when_true);
        return;
    }
    if (expression->kind == EXPRESSION_BINARY && (expression->binary.operator == OP_AND || expression->binary.operator == OP_OR)) {
        jump_if_logical(context, expression, target, when_true);
        return;
    }
    if (expression->kind == EXPRESSION_BINARY && is_comparison(expression->binary.operator)) {
        jump_if_comparison(context, expression, target, when_true);
        return;
    }
    emit_int(context, expression);
    emit(context, "test rax, rax");
    emit(context, "%s %s", when_true ? "jnz" : "jz", target);
}

void emit_condition_value(FunctionContext *context, Expression *expression) {
    Expression *left = expression->binary.left;
    Expression *right = expression->binary.right;
    ValueType operation = expression->binary.operation;
    if (is_comparison(expression->binary.operator) && !is_floating(operation)) {
        Operand operand = emit_int_pair(context, left, right);
        emit(context, "cmp rax, %s", operand.text);
        emit(context, "set%s al", int_condition(expression->binary.operator, true, operation == TYPE_ULONG));
        emit(context, "movzx eax, al");
        return;
    }
    const char *true_label = new_label(context, "true");
    const char *end_label = new_label(context, "end");
    jump_if(context, expression, true_label, true);
    emit(context, "xor eax, eax");
    emit(context, "jmp %s", end_label);
    emit_label(context, true_label);
    emit(context, "mov eax, 1");
    emit_label(context, end_label);
}
