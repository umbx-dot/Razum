#include "codegen/codegen.h"

#include "analysis/analysis.h"

enum { SHADOW_SPACE = 32, SLOT_SIZE = 8, PAGE_SIZE = 4096 };

static const char *const INT_REGISTERS[] = {"rcx", "rdx", "r8", "r9"};

static void emit_signature(TextBuffer *output, const Function *function) {
    text_append(output, "\n; ");
    text_append(output, type_name(function->return_type));
    text_append(output, " ");
    text_append(output, function->name);
    text_append(output, "(");
    for (int index = 0; index < function->parameters.count; index++) {
        Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        text_append(output, index > 0 ? ", " : "");
        text_append(output, describe_shape(parameter->shape));
        text_append(output, " ");
        text_append(output, parameter->name);
    }
    text_line(output, ")");
}

static int emit_frame(TextBuffer *output, const Function *function, const FunctionContext *context) {
    int offset = SHADOW_SPACE + SLOT_SIZE * context->stack_argument_count;
    int size = offset + SLOT_SIZE * (context->temp_count + context->saved_count) + 2 * SLOT_SIZE * context->saved_vector_count;
    for (int index = 0; index < function->locals.count; index++) {
        size += LIST_AT(function->locals, Symbol, index)->size;
    }
    while (size % 16 != 8) {
        size += SLOT_SIZE;
    }
    text_line(output, ".@frame equ %d", size);
    for (int index = 0; index < function->locals.count; index++) {
        const Symbol *local = LIST_AT(function->locals, Symbol, index);
        text_line(output, "%s equ %d", local->label, offset);
        offset += local->size;
    }
    for (int index = 0; index < context->temp_count; index++) {
        text_line(output, ".@t%d equ %d", index, offset);
        offset += SLOT_SIZE;
    }
    for (int index = 0; index < context->saved_count; index++) {
        text_line(output, ".@save%d equ %d", index, offset);
        offset += SLOT_SIZE;
    }
    for (int index = 0; index < context->saved_vector_count; index++) {
        text_line(output, ".@vsave%d equ %d", index, offset);
        offset += 2 * SLOT_SIZE;
    }
    for (int index = 0; index < function->parameters.count; index++) {
        const Symbol *symbol = LIST_AT(function->parameters, Parameter, index)->symbol;
        text_line(output, "%s equ %d", symbol->label, size + 8 + SLOT_SIZE * symbol->index);
        if (symbol->length_label != NULL) {
            text_line(output, "%s equ %d", symbol->length_label, size + 8 + SLOT_SIZE * (symbol->index + 1));
        }
    }
    return size;
}

static void spill(TextBuffer *output, int position, bool is_floating) {
    if (position >= 4) {
        return;
    }
    if (is_floating) {
        text_line(output, "    movsd qword [rsp + %d], xmm%d", 8 + SLOT_SIZE * position, position);
    } else {
        text_line(output, "    mov qword [rsp + %d], %s", 8 + SLOT_SIZE * position, INT_REGISTERS[position]);
    }
}

static void emit_parameter_spills(TextBuffer *output, const Function *function) {
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        if (parameter->symbol->register_name == NULL) {
            spill(output, parameter->symbol->index, is_floating(parameter->type));
        }
        if (parameter->symbol->length_label != NULL) {
            spill(output, parameter->symbol->index + 1, false);
        }
    }
}

static const char *extend_from_slot(ValueType type) {
    switch (type) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_BYTE: return "movzx eax, byte";
        case TYPE_SBYTE: return "movsx rax, byte";
        case TYPE_SHORT: return "movsx rax, word";
        case TYPE_USHORT: return "movzx eax, word";
        case TYPE_INT: return "movsxd rax, dword";
        default: return "mov eax, dword";
    }
}

static void emit_parameter_extension(TextBuffer *output, const Function *function) {
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        const Symbol *symbol = parameter->symbol;
        bool is_narrow = parameter->type == TYPE_BOOL || (is_integer(parameter->type) && type_info(parameter->type)->size < 8);
        if (is_narrow && (symbol->register_name == NULL || symbol->index >= 4)) {
            int offset = 8 + SLOT_SIZE * parameter->symbol->index;
            text_line(output, "    %s [rsp + %d]", extend_from_slot(parameter->type), offset);
            text_line(output, "    mov qword [rsp + %d], rax", offset);
        }
    }
}

static void emit_stack_reservation(TextBuffer *output, int size) {
    if (size <= PAGE_SIZE) {
        text_line(output, "    sub rsp, .@frame");
        return;
    }
    text_line(output, "    mov eax, %d", size / PAGE_SIZE);
    text_line(output, ".@probe:");
    text_line(output, "    sub rsp, %d", PAGE_SIZE);
    text_line(output, "    mov qword [rsp], 0");
    text_line(output, "    dec eax");
    text_line(output, "    jnz .@probe");
    if (size % PAGE_SIZE != 0) {
        text_line(output, "    sub rsp, %d", size % PAGE_SIZE);
    }
}

static void emit_register_setup(TextBuffer *output, const Function *function, const FunctionContext *context) {
    for (int index = 0; index < context->saved_count; index++) {
        text_line(output, "    mov qword [rsp + .@save%d], %s", index, context->saved_registers[index]);
    }
    for (int index = 0; index < context->saved_vector_count; index++) {
        text_line(output, "    movups [rsp + .@vsave%d], %s", index, context->saved_vectors[index]);
    }
    for (int index = 0; index < function->parameters.count; index++) {
        const Symbol *symbol = LIST_AT(function->parameters, Parameter, index)->symbol;
        if (symbol->register_name == NULL) {
            continue;
        }
        if (symbol->index < 4) {
            if (is_floating(symbol->type)) {
                text_line(output, "    mov%s %s, xmm%d", symbol->type == TYPE_FLOAT ? "ss" : "sd",
                          symbol->register_name, symbol->index);
            } else {
                const char *source = INT_REGISTERS[symbol->index];
                if ((function->address_taken || function->is_exported) && type_info(symbol->type)->size < 8) {
                    static const char *const LOW_BYTES[] = {"cl", "dl", "r8b", "r9b"};
                    static const char *const LOW_WORDS[] = {"cx", "dx", "r8w", "r9w"};
                    static const char *const LOW_DWORDS[] = {"ecx", "edx", "r8d", "r9d"};
                    int size = type_info(symbol->type)->size;
                    const char *part = size == 1 ? LOW_BYTES[symbol->index] : size == 2 ? LOW_WORDS[symbol->index]
                                                                                     : LOW_DWORDS[symbol->index];
                    if (size == 4 && !type_info(symbol->type)->is_signed) {
                        text_line(output, "    mov eax, %s", part);
                        text_line(output, "    mov %s, rax", symbol->register_name);
                    } else {
                        const char *op = size == 4 ? "movsxd" : type_info(symbol->type)->is_signed ? "movsx" : "movzx";
                        text_line(output, "    %s %s, %s", op, symbol->register_name, part);
                    }
                } else {
                    text_line(output, "    mov %s, %s", symbol->register_name, source);
                }
            }
        } else if (symbol->type == TYPE_DOUBLE) {
            text_line(output, "    movsd %s, qword [rsp + %s]", symbol->register_name, symbol->label);
        } else if (symbol->type == TYPE_FLOAT) {
            text_line(output, "    movss %s, dword [rsp + %s]", symbol->register_name, symbol->label);
        } else {
            text_line(output, "    mov %s, qword [rsp + %s]", symbol->register_name, symbol->label);
        }
    }
}

void emit_function(ProgramContext *program, Function *function, TextBuffer *output) {
    FunctionContext context = {0};
    context.program = program;
    context.function = function;
    assign_registers(&context);
    context.loop_vector_base = context.saved_vector_count;
    setup_tail_recursion(&context);
    if (context.tail_sum != NULL) {
        emit(&context, "xor %s, %s", context.tail_sum, context.tail_sum);
    }
    if (context.tail_label != NULL) {
        emit_label(&context, context.tail_label);
    }
    emit_statement(&context, function->body);
    if (!always_returns(function->body)) {
        emit_epilogue(&context);
    }
    emit_signature(output, function);
    text_line(output, "%s%s:", function_label_prefix(program), function->name);
    int size = emit_frame(output, function, &context);
    emit_fast_return(&context, output);
    emit_parameter_spills(output, function);
    if (function->is_exported || function->address_taken) {
        emit_parameter_extension(output, function);
    }
    emit_stack_reservation(output, size);
    emit_register_setup(output, function, &context);
    text_append_bytes(output, context.body.data, context.body.length);
}
