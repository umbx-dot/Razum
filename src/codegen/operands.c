#include "codegen/codegen.h"

#include <inttypes.h>
#include <stdio.h>

Operand sized_operand(const Symbol *symbol, const char *size) {
    Operand operand = {0};
    if (symbol->register_name != NULL) {
        snprintf(operand.text, sizeof operand.text, "%s", symbol->register_name);
    } else if (symbol->kind == SYMBOL_GLOBAL) {
        snprintf(operand.text, sizeof operand.text, "%s [%s]", size, symbol->label);
    } else {
        snprintf(operand.text, sizeof operand.text, "%s [rsp + %s]", size, symbol->label);
    }
    return operand;
}

Operand memory_operand(const Symbol *symbol) {
    return sized_operand(symbol, "qword");
}

bool int_operand(const Expression *expression, Operand *operand) {
    if (expression->kind == EXPRESSION_INT && expression->int_literal.value >= INT32_MIN && expression->int_literal.value <= INT32_MAX) {
        operand->is_address = false;
        snprintf(operand->text, sizeof operand->text, "%" PRId64, expression->int_literal.value);
        return true;
    }
    if (expression->kind == EXPRESSION_NAME && !is_floating(expression->type) && !is_aggregate(expression->type) &&
        !is_memory_symbol(expression->name.symbol)) {
        *operand = memory_operand(expression->name.symbol);
        return true;
    }
    return false;
}

bool double_operand(ProgramContext *program, const Expression *expression, Operand *operand) {
    if (expression->kind == EXPRESSION_DOUBLE) {
        operand->is_address = false;
        snprintf(operand->text, sizeof operand->text, "qword [num@%d]", number_constant_index(program, expression->double_literal));
        return true;
    }
    if (expression->kind == EXPRESSION_NAME && expression->type == TYPE_DOUBLE) {
        *operand = memory_operand(expression->name.symbol);
        return true;
    }
    return false;
}

Operand string_address(FunctionContext *context, const char *bytes, size_t length) {
    Operand operand = {0};
    operand.is_address = true;
    snprintf(operand.text, sizeof operand.text, "[str@%d]", string_constant_index(context->program, bytes, length));
    return operand;
}

bool is_immediate(const Operand *operand) {
    return !operand->is_address && ((operand->text[0] >= '0' && operand->text[0] <= '9') || operand->text[0] == '-');
}

bool is_memory_symbol(const Symbol *symbol) {
    return symbol->address_taken && is_integer(symbol->type) && type_info(symbol->type)->size < 8;
}
