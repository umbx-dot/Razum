#include "codegen/codegen.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"

static const char *const INT_REGISTERS[] = {"rcx", "rdx", "r8", "r9"};

typedef struct {
    Operand operand;
    bool is_floating;
} Argument;

static bool emit_single_scalar_call(FunctionContext *context, Expression *call) {
    Function *target = call->call.target;
    if (!context->program->optimize || target == NULL || target->extern_dll != NULL || call->call.arguments.count != 1) {
        return false;
    }
    const Parameter *parameter = LIST_AT(target->parameters, Parameter, 0);
    if (is_aggregate(parameter->type)) {
        return false;
    }
    Expression *argument = LIST_AT(call->call.arguments, Expression, 0);
    Operand operand;
    if (is_floating(parameter->type)) {
        emit_value(context, argument, parameter->type);
    } else if (argument->kind == EXPRESSION_STRING) {
        operand = string_address(context, argument->string.bytes, argument->string.length);
        emit(context, "lea rcx, %s", operand.text);
    } else if (int_operand(argument, &operand)) {
        emit(context, "mov rcx, %s", operand.text);
    } else if (!emit_wide_tree(context, argument, "rcx")) {
        emit_int(context, argument);
        emit(context, "mov rcx, rax");
    }
    emit(context, "call %s%s", function_label_prefix(context->program), target->name);
    return true;
}

static Operand into_temp(FunctionContext *context, const char *instruction, const char *source, int *temps) {
    Operand operand = acquire_temp(context);
    (*temps)++;
    emit(context, "%s %s, %s", instruction, operand.text, source);
    return operand;
}

static Operand address_operand(FunctionContext *context, Expression *argument, int *temps) {
    Operand operand = {0};
    char place[OPERAND_SIZE - 2];
    if (place_text(argument, place, sizeof place)) {
        operand.is_address = true;
        snprintf(operand.text, sizeof operand.text, "[%s]", place);
        return operand;
    }
    emit_address(context, argument);
    return into_temp(context, "mov", "rax", temps);
}

static Operand length_operand(const Expression *argument) {
    Operand operand = {0};
    if (argument->shape->count >= 0) {
        snprintf(operand.text, sizeof operand.text, "%d", argument->shape->count);
    } else {
        snprintf(operand.text, sizeof operand.text, "qword [rsp + %s]", argument->name.symbol->length_label);
    }
    return operand;
}

static Operand scalar_operand(FunctionContext *context, Expression *argument, ValueType parameter_type, int *temps) {
    Operand operand = {0};
    if (argument->kind == EXPRESSION_STRING) {
        return string_address(context, argument->string.bytes, argument->string.length);
    }
    if (is_aggregate(argument->type)) {
        return address_operand(context, argument, temps);
    }
    if (is_floating(parameter_type)) {
        if (argument->type == parameter_type && argument->kind == EXPRESSION_NAME) {
            return memory_operand(argument->name.symbol);
        }
        if (argument->type == TYPE_DOUBLE && parameter_type == TYPE_DOUBLE && double_operand(context->program, argument, &operand)) {
            return operand;
        }
        emit_value(context, argument, parameter_type);
        return into_temp(context, "movsd", "xmm0", temps);
    }
    if (int_operand(argument, &operand)) {
        return operand;
    }
    emit_int(context, argument);
    return into_temp(context, "mov", "rax", temps);
}

static void store_stack_argument(FunctionContext *context, int position, const Operand *operand) {
    int offset = 32 + 8 * (position - 4);
    if (is_immediate(operand)) {
        emit(context, "mov qword [rsp + %d], %s", offset, operand->text);
        return;
    }
    if (strncmp(operand->text, "xmm", 3) == 0) {
        emit(context, "movsd qword [rsp + %d], %s", offset, operand->text);
        return;
    }
    emit(context, "%s rax, %s", operand->is_address ? "lea" : "mov", operand->text);
    emit(context, "mov qword [rsp + %d], rax", offset);
}

static void load_register_argument(FunctionContext *context, int position, const Argument *argument) {
    if (argument->is_floating) {
        emit(context, "movsd xmm%d, %s", position, argument->operand.text);
    } else {
        emit(context, "%s %s, %s", argument->operand.is_address ? "lea" : "mov", INT_REGISTERS[position], argument->operand.text);
    }
}

void emit_call(FunctionContext *context, Expression *call) {
    if (call->call.is_free) {
        emit_int(context, LIST_AT(call->call.arguments, Expression, 0));
        emit(context, "mov rcx, rax");
        emit(context, "call rz@release");
        return;
    }
    if (emit_single_scalar_call(context, call)) {
        return;
    }
    Function *target = call->call.target;
    Expression *callee = call->call.callee;
    int count = call->call.arguments.count;
    Argument *arguments = arena_alloc(sizeof *arguments * (size_t)(2 * count + 1));
    int positions = 0;
    int temps = 0;
    Operand callee_operand = {0};
    if (callee != NULL && callee->kind == EXPRESSION_NAME) {
        callee_operand = memory_operand(callee->name.symbol);
    } else if (callee != NULL) {
        emit_int(context, callee);
        callee_operand = into_temp(context, "mov", "rax", &temps);
    }
    for (int index = 0; index < count; index++) {
        const Shape *shape = target != NULL ? LIST_AT(target->parameters, Parameter, index)->shape
                                            : LIST_AT(callee->shape->parameters, Shape, index);
        Expression *argument = LIST_AT(call->call.arguments, Expression, index);
        Argument *slot = &arguments[positions++];
        slot->is_floating = is_floating(shape->kind);
        if (is_aggregate(shape->kind) && argument->type == TYPE_POINTER) {
            slot->operand = scalar_operand(context, argument, TYPE_POINTER, &temps);
        } else if (is_aggregate(shape->kind)) {
            slot->operand = address_operand(context, argument, &temps);
            const Parameter *parameter = target != NULL ? LIST_AT(target->parameters, Parameter, index) : NULL;
            bool passes_length = parameter != NULL ? target->extern_dll == NULL && parameter->symbol->length_label != NULL
                                                   : shape->kind == TYPE_ARRAY && shape->count < 0;
            if (passes_length) {
                arguments[positions++] = (Argument){length_operand(argument), false};
            }
        } else {
            slot->operand = scalar_operand(context, argument, shape->kind, &temps);
        }
    }
    if (positions - 4 > context->stack_argument_count) {
        context->stack_argument_count = positions - 4;
    }
    for (int position = 4; position < positions; position++) {
        store_stack_argument(context, position, &arguments[position].operand);
    }
    for (int position = 0; position < positions && position < 4; position++) {
        load_register_argument(context, position, &arguments[position]);
    }
    if (callee != NULL) {
        emit(context, "call %s", callee_operand.text);
    } else if (target->extern_dll != NULL) {
        emit(context, "call [iat@%s@%s]", target->extern_dll, target->name);
    } else {
        emit(context, "call %s%s", function_label_prefix(context->program), target->name);
    }
    for (int index = 0; index < temps; index++) {
        release_temp(context);
    }
}
