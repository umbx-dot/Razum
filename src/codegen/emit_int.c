#include "codegen/codegen.h"

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/diagnostics.h"

const char *integer_arithmetic_instruction(Operator operator) {
    switch (operator) {
        case OP_ADD: return "add";
        case OP_SUBTRACT: return "sub";
        case OP_MULTIPLY: return "imul";
        case OP_BIT_AND: return "and";
        case OP_BIT_OR: return "or";
        case OP_BIT_XOR: return "xor";
        default: return NULL;
    }
}

bool is_shift(Operator operator) {
    return operator == OP_SHIFT_LEFT || operator == OP_SHIFT_RIGHT;
}

const char *shift_instruction(Operator operator, ValueType type) {
    if (operator == OP_SHIFT_LEFT) {
        return "shl";
    }
    return type_info(type)->is_signed ? "sar" : "shr";
}

static void emit_int_shift(FunctionContext *context, Expression *expression) {
    ValueType type = expression->type;
    Operand operand = emit_int_pair(context, expression->binary.left, expression->binary.right);
    const char *target = type_info(type)->size == 8 ? "rax" : "eax";
    const char *instruction = shift_instruction(expression->binary.operator, type);
    if (is_immediate(&operand)) {
        long long count = strtoll(operand.text, NULL, 10) & (type_info(type)->size == 8 ? 63 : 31);
        emit(context, "%s %s, %lld", instruction, target, count);
    } else if (context->program->zen3 && type_info(type)->size == 8) {
        if (operand.text[0] == 'q') {
            emit(context, "mov rcx, %s", operand.text);
            snprintf(operand.text, sizeof operand.text, "rcx");
        }
        const char *bmi = expression->binary.operator == OP_SHIFT_LEFT ? "shlx"
                          : type_info(type)->is_signed                 ? "sarx"
                                                                       : "shrx";
        emit(context, "%s rax, rax, %s", bmi, operand.text);
    } else {
        if (strcmp(operand.text, "rcx") != 0) {
            emit(context, "mov rcx, %s", operand.text);
        }
        emit(context, "%s %s, cl", instruction, target);
    }
    emit_narrow(context, type);
}

Operand emit_int_pair(FunctionContext *context, Expression *left, Expression *right) {
    Operand operand = {0};
    bool simple = int_operand(right, &operand);
    emit_int(context, left);
    if (simple) {
        return operand;
    }
    if (context->program->optimize && emit_wide_tree(context, right, "rcx")) {
        snprintf(operand.text, sizeof operand.text, "rcx");
        return operand;
    }
    Operand temp = acquire_temp(context);
    emit(context, "mov %s, rax", temp.text);
    emit_int(context, right);
    emit(context, "mov rcx, rax");
    emit(context, "mov rax, %s", temp.text);
    release_temp(context);
    snprintf(operand.text, sizeof operand.text, "rcx");
    return operand;
}

static void emit_int_arithmetic(FunctionContext *context, Expression *expression) {
    Expression *right = expression->binary.right;
    if (context->program->optimize && expression->binary.operator == OP_DIVIDE &&
        (expression->type == TYPE_LONG || expression->type == TYPE_ULONG) &&
        right->kind == EXPRESSION_INT && right->int_literal.value > 0) {
        uint64_t divisor = (uint64_t)right->int_literal.value;
        if ((divisor & (divisor - 1)) == 0) {
            emit_int(context, expression->binary.left);
            int shift = 0;
            for (; divisor > 1; divisor >>= 1) {
                shift++;
            }
            if (shift != 0 && expression->type == TYPE_LONG) {
                emit(context, "mov rcx, rax");
                emit(context, "sar rcx, 63");
                emit(context, "shr rcx, %d", 64 - shift);
                emit(context, "add rax, rcx");
                emit(context, "sar rax, %d", shift);
            } else if (shift != 0) {
                emit(context, "shr rax, %d", shift);
            }
            return;
        }
    }
    Operand operand = emit_int_pair(context, expression->binary.left, expression->binary.right);
    const char *instruction = integer_arithmetic_instruction(expression->binary.operator);
    if (instruction != NULL) {
        emit(context, "%s rax, %s", instruction, operand.text);
        emit_narrow(context, expression->type);
        return;
    }
    if (strcmp(operand.text, "rcx") != 0) {
        emit(context, "mov rcx, %s", operand.text);
    }
    emit_divide(context, expression->type);
    if (expression->binary.operator == OP_REMAINDER) {
        emit(context, "mov rax, rdx");
    }
}

static void emit_int_cast(FunctionContext *context, Expression *operand, ValueType target) {
    if (is_floating(operand->type)) {
        emit_double(context, operand);
        emit_double_to_int(context, target);
        return;
    }
    emit_int(context, operand);
    if (target == TYPE_BOOL && operand->type != TYPE_BOOL) {
        emit(context, "test rax, rax");
        emit(context, "setne al");
        emit(context, "movzx eax, al");
    } else if (operand->type != TYPE_BOOL && !widens(operand->type, target)) {
        emit_narrow(context, target);
    }
}

static void emit_int_unary(FunctionContext *context, Expression *expression) {
    emit_int(context, expression->unary.operand);
    if (expression->unary.operator == OP_NEGATE || expression->unary.operator == OP_BIT_NOT) {
        emit(context, "%s rax", expression->unary.operator == OP_NEGATE ? "neg" : "not");
        emit_narrow(context, expression->type);
        return;
    }
    emit(context, "test rax, rax");
    emit(context, "sete al");
    emit(context, "movzx eax, al");
}

void emit_place_load(FunctionContext *context, Expression *expression) {
    if (expression->kind == EXPRESSION_FIELD && expression->field.is_length) {
        emit_length(context, expression);
        return;
    }
    char place[OPERAND_SIZE];
    if (place_text(expression, place, sizeof place)) {
        emit_load_from(context, expression->type, place);
        return;
    }
    emit_address(context, expression);
    emit_load_from(context, expression->type, "rax");
}

void emit_int(FunctionContext *context, Expression *expression) {
    if (is_aggregate(expression->type)) {
        emit_address(context, expression);
        return;
    }
    switch (expression->kind) {
        case EXPRESSION_INDEX:
        case EXPRESSION_FIELD:
        case EXPRESSION_DEREF:
            emit_place_load(context, expression);
            return;
        case EXPRESSION_CONDITIONAL:
            emit_conditional(context, expression, false);
            return;
        case EXPRESSION_ADDRESS:
            if (expression->unary.operand->kind == EXPRESSION_FUNCTION) {
                emit_int(context, expression->unary.operand);
            } else {
                emit_address(context, expression->unary.operand);
            }
            return;
        case EXPRESSION_NEW:
            emit_new(context, expression);
            return;
        case EXPRESSION_FUNCTION:
            if (expression->function_value->extern_dll != NULL) {
                emit(context, "mov rax, qword [iat@%s@%s]", expression->function_value->extern_dll, expression->function_value->name);
            } else {
                emit(context, "lea rax, [%s%s]", function_label_prefix(context->program), expression->function_value->name);
            }
            return;
        case EXPRESSION_INT:
            if (expression->int_literal.value == 0) {
                emit(context, "xor eax, eax");
            } else {
                emit(context, "mov rax, %" PRId64, expression->int_literal.value);
            }
            return;
        case EXPRESSION_NAME:
            if (is_memory_symbol(expression->name.symbol)) {
                emit_place_load(context, expression);
            } else {
                emit(context, "mov rax, %s", memory_operand(expression->name.symbol).text);
            }
            return;
        case EXPRESSION_STRING:
            emit(context, "lea rax, %s", string_address(context, expression->string.bytes, expression->string.length).text);
            return;
        case EXPRESSION_CALL:
            emit_call(context, expression);
            emit_narrow(context, expression->type);
            return;
        case EXPRESSION_CAST:
            emit_int_cast(context, expression->cast.operand, expression->type);
            return;
        case EXPRESSION_UNARY:
            emit_int_unary(context, expression);
            return;
        case EXPRESSION_BINARY:
            if (context->program->optimize && emit_wide_tree(context, expression, "rax")) {
                return;
            }
            if (is_shift(expression->binary.operator)) {
                emit_int_shift(context, expression);
            } else if ((expression->binary.operator >= OP_ADD && expression->binary.operator <= OP_REMAINDER) ||
                       (expression->binary.operator >= OP_BIT_AND && expression->binary.operator <= OP_BIT_XOR)) {
                emit_int_arithmetic(context, expression);
            } else {
                emit_condition_value(context, expression);
            }
            return;
        default:
            fatal_error("internal: expression kind %d has no int value", expression->kind);
    }
}
