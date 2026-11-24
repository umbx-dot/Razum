#include "codegen/codegen.h"

#include <stdio.h>

static const char *int_compound_instruction(Operator operator) {
    switch (operator) {
        case OP_ADD: return "add";
        case OP_SUBTRACT: return "sub";
        case OP_BIT_AND: return "and";
        case OP_BIT_OR: return "or";
        case OP_BIT_XOR: return "xor";
        default: return NULL;
    }
}

static void emit_shift_compound(FunctionContext *context, const Symbol *symbol, Operator operator, Expression *value) {
    Operand target = memory_operand(symbol);
    emit_int(context, value);
    emit(context, "mov rcx, rax");
    const char *instruction = shift_instruction(operator, symbol->type);
    if (type_info(symbol->type)->size == 8) {
        emit(context, "%s %s, cl", instruction, target.text);
        return;
    }
    emit(context, "mov rax, %s", target.text);
    emit(context, "%s eax, cl", instruction);
    emit_narrow(context, symbol->type);
    emit(context, "mov %s, rax", target.text);
}

static const char *double_instruction(Operator operator) {
    switch (operator) {
        case OP_ADD: return "addsd";
        case OP_SUBTRACT: return "subsd";
        case OP_MULTIPLY: return "mulsd";
        default: return "divsd";
    }
}

static void emit_store(FunctionContext *context, const Symbol *symbol, Expression *value) {
    if (is_floating(symbol->type)) {
        if (context->program->optimize && emit_double_tree_to_symbol(context, value, symbol)) {
            return;
        }
        emit_double(context, value);
        emit_store_floating(context, symbol);
        return;
    }
    Operand target = memory_operand(symbol);
    Operand operand = {0};
    if (int_operand(value, &operand) && is_immediate(&operand)) {
        emit(context, "mov %s, %s", target.text, operand.text);
        return;
    }
    emit_int(context, value);
    emit(context, "mov %s, rax", target.text);
}

static void emit_wide_compound(FunctionContext *context, const Symbol *symbol, Operator operator, Expression *value) {
    Operand target = memory_operand(symbol);
    const char *instruction = int_compound_instruction(operator);
    Operand operand = {0};
    if (instruction != NULL && int_operand(value, &operand) && is_immediate(&operand)) {
        emit(context, "%s %s, %s", instruction, target.text, operand.text);
        return;
    }
    emit_int(context, value);
    if (instruction != NULL) {
        emit(context, "%s %s, rax", instruction, target.text);
    } else if (operator == OP_MULTIPLY) {
        emit(context, "imul rax, %s", target.text);
        emit(context, "mov %s, rax", target.text);
    } else {
        emit(context, "mov rcx, rax");
        emit(context, "mov rax, %s", target.text);
        emit_divide(context, symbol->type);
        emit(context, "mov %s, %s", target.text, operator == OP_REMAINDER ? "rdx" : "rax");
    }
}

static void emit_narrow_compound(FunctionContext *context, const Symbol *symbol, Operator operator, Expression *value) {
    Operand target = memory_operand(symbol);
    const char *instruction = int_compound_instruction(operator);
    Operand operand = {0};
    if (!int_operand(value, &operand) || (instruction == NULL && operator != OP_MULTIPLY)) {
        emit_int(context, value);
        emit(context, "mov rcx, rax");
        snprintf(operand.text, sizeof operand.text, "rcx");
    }
    emit(context, "mov rax, %s", target.text);
    if (instruction != NULL) {
        emit(context, "%s rax, %s", instruction, operand.text);
    } else if (operator == OP_MULTIPLY) {
        emit(context, "imul rax, %s", operand.text);
    } else {
        emit_divide(context, symbol->type);
        if (operator == OP_REMAINDER) {
            emit(context, "mov rax, rdx");
        }
    }
    emit_narrow(context, symbol->type);
    emit(context, "mov %s, rax", target.text);
}

static void emit_double_compound(FunctionContext *context, const Symbol *symbol, Operator operator, Expression *value) {
    emit_double(context, value);
    emit(context, "movsd xmm1, xmm0");
    emit_load_floating(context, symbol, "xmm0");
    emit(context, "%s xmm0, xmm1", double_instruction(operator));
    emit_store_floating(context, symbol);
}

static void emit_increment(FunctionContext *context, const Symbol *symbol, bool is_increment) {
    Operand target = memory_operand(symbol);
    const char *instruction = is_increment ? "inc" : "dec";
    if (is_floating(symbol->type)) {
        emit_load_floating(context, symbol, "xmm0");
        emit(context, "%s xmm0, qword [num@%d]", is_increment ? "addsd" : "subsd", number_constant_index(context->program, 1.0));
        emit_store_floating(context, symbol);
    } else if (type_info(symbol->type)->size == 8) {
        emit(context, "%s %s", instruction, target.text);
    } else {
        emit(context, "mov rax, %s", target.text);
        emit(context, "%s rax", instruction);
        emit_narrow(context, symbol->type);
        emit(context, "mov %s, rax", target.text);
    }
}

void emit_assignment(FunctionContext *context, Statement *statement) {
    switch (statement->kind) {
        case STATEMENT_DECLARATION:
            if (is_aggregate(statement->declaration.type)) {
                emit_aggregate_declaration(context, statement);
            } else if (statement->declaration.initial == NULL && statement->declaration.type == TYPE_STRING) {
                emit(context, "lea rax, %s", string_address(context, "", 0).text);
                emit(context, "mov %s, rax", memory_operand(statement->declaration.symbol).text);
            } else if (statement->declaration.initial == NULL) {
                emit(context, "mov %s, 0", memory_operand(statement->declaration.symbol).text);
            } else {
                emit_store(context, statement->declaration.symbol, statement->declaration.initial);
            }
            return;
        case STATEMENT_INCREMENT:
            if (statement->increment.target->kind == EXPRESSION_NAME && !is_memory_symbol(statement->increment.target->name.symbol)) {
                emit_increment(context, statement->increment.target->name.symbol, statement->increment.is_increment);
            } else {
                emit_place_increment(context, statement->increment.target, statement->increment.is_increment);
            }
            return;
        default:
            break;
    }
    Expression *target = statement->assignment.target;
    Operator operator = statement->assignment.operator;
    if (is_aggregate(target->type)) {
        emit_copy(context, target, statement->assignment.value);
        return;
    }
    if (target->kind != EXPRESSION_NAME || is_memory_symbol(target->name.symbol)) {
        if (operator == OP_ASSIGN) {
            emit_place_store(context, target, statement->assignment.value);
        } else {
            emit_place_compound(context, target, operator, statement->assignment.value);
        }
        return;
    }
    const Symbol *symbol = target->name.symbol;
    if (operator == OP_ASSIGN) {
        emit_store(context, symbol, statement->assignment.value);
    } else if (is_shift(operator)) {
        emit_shift_compound(context, symbol, operator, statement->assignment.value);
    } else if (is_floating(symbol->type)) {
        emit_double_compound(context, symbol, operator, statement->assignment.value);
    } else if (type_info(symbol->type)->size == 8) {
        emit_wide_compound(context, symbol, operator, statement->assignment.value);
    } else {
        emit_narrow_compound(context, symbol, operator, statement->assignment.value);
    }
}
