#include "codegen/codegen.h"

#include <stdio.h>

static const char *integer_instruction(Operator operator) {
    switch (operator) {
        case OP_ADD: return "add";
        case OP_SUBTRACT: return "sub";
        case OP_BIT_AND: return "and";
        case OP_BIT_OR: return "or";
        case OP_BIT_XOR: return "xor";
        default: return NULL;
    }
}

static const char *floating_instruction(Operator operator) {
    switch (operator) {
        case OP_ADD: return "addsd";
        case OP_SUBTRACT: return "subsd";
        case OP_MULTIPLY: return "mulsd";
        default: return "divsd";
    }
}

static void address_into_r8(FunctionContext *context, Expression *target, char *place, size_t size) {
    if (!place_text(target, place, size)) {
        emit_address(context, target);
        emit(context, "mov r8, rax");
        snprintf(place, size, "r8");
    }
}

void emit_place_store(FunctionContext *context, Expression *target, Expression *value) {
    ValueType type = target->type;
    char place[OPERAND_SIZE];
    Operand operand = {0};
    bool is_constant = !is_floating(type) && int_operand(value, &operand) && is_immediate(&operand);
    if (place_text(target, place, sizeof place)) {
        if (is_constant) {
            emit(context, "mov %s [%s], %s", size_word(type), place, operand.text);
        } else if (is_floating(type)) {
            emit_double(context, value);
            emit_store_to(context, type, place, false);
        } else {
            emit_int(context, value);
            emit_store_to(context, type, place, false);
        }
        return;
    }
    if (is_constant) {
        emit_address(context, target);
        emit(context, "mov %s [rax], %s", size_word(type), operand.text);
        return;
    }
    Operand temp = acquire_temp(context);
    if (is_floating(type)) {
        emit_double(context, value);
        emit(context, "movsd %s, xmm0", temp.text);
        emit_address(context, target);
        emit(context, "movsd xmm0, %s", temp.text);
        emit_store_to(context, type, "rax", false);
    } else {
        emit_int(context, value);
        emit(context, "mov %s, rax", temp.text);
        emit_address(context, target);
        emit(context, "mov rcx, %s", temp.text);
        emit_store_to(context, type, "rax", true);
    }
    release_temp(context);
}

static void emit_floating_compound(FunctionContext *context, Expression *target, Operator operator, Expression *value) {
    char place[OPERAND_SIZE];
    Operand temp = acquire_temp(context);
    emit_double(context, value);
    emit(context, "movsd %s, xmm0", temp.text);
    address_into_r8(context, target, place, sizeof place);
    emit_load_from(context, target->type, place);
    emit(context, "%s xmm0, %s", floating_instruction(operator), temp.text);
    emit_store_to(context, target->type, place, false);
    release_temp(context);
}

void emit_place_compound(FunctionContext *context, Expression *target, Operator operator, Expression *value) {
    ValueType type = target->type;
    if (is_floating(type)) {
        emit_floating_compound(context, target, operator, value);
        return;
    }
    char place[OPERAND_SIZE];
    const char *instruction = integer_instruction(operator);
    Operand operand = {0};
    bool is_constant = int_operand(value, &operand) && is_immediate(&operand);
    Operand temp = {0};
    if (!is_constant) {
        temp = acquire_temp(context);
        emit_int(context, value);
        emit(context, "mov %s, rax", temp.text);
    }
    address_into_r8(context, target, place, sizeof place);
    if (instruction != NULL && is_constant) {
        emit(context, "%s %s [%s], %s", instruction, size_word(type), place, operand.text);
        return;
    }
    if (is_constant) {
        emit(context, "mov rcx, %s", operand.text);
    } else {
        emit(context, "mov rcx, %s", temp.text);
        release_temp(context);
    }
    if (instruction != NULL) {
        emit(context, "%s %s [%s], %s", instruction, size_word(type), place, sized_register(type, true));
        return;
    }
    if (is_shift(operator)) {
        emit(context, "%s %s [%s], cl", shift_instruction(operator, type), size_word(type), place);
        return;
    }
    emit_load_from(context, type, place);
    if (operator == OP_MULTIPLY) {
        emit(context, "imul rax, rcx");
    } else {
        emit_divide(context, type == TYPE_ULONG ? TYPE_ULONG : TYPE_LONG);
        if (operator == OP_REMAINDER) {
            emit(context, "mov rax, rdx");
        }
    }
    emit_store_to(context, type, place, false);
}

void emit_place_increment(FunctionContext *context, Expression *target, bool is_increment) {
    char place[OPERAND_SIZE];
    address_into_r8(context, target, place, sizeof place);
    if (!is_floating(target->type)) {
        emit(context, "%s %s [%s]", is_increment ? "inc" : "dec", size_word(target->type), place);
        return;
    }
    emit_load_from(context, target->type, place);
    emit(context, "%s xmm0, qword [num@%d]", is_increment ? "addsd" : "subsd", number_constant_index(context->program, 1.0));
    emit_store_to(context, target->type, place, false);
}
