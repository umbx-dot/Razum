#include "codegen/codegen.h"

static const char *const SYSTEM_V_INTEGER[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static const char *const RAZUM_INTEGER[] = {"rcx", "rdx", "r8", "r9"};

static void stage_argument(TextBuffer *output, int slot, bool floating, int *ints, int *vectors, int *stack) {
    if (floating && *vectors < 8) {
        text_line(output, "    movq [rsp + %d], xmm%d", 8 * slot, (*vectors)++);
    } else if (!floating && *ints < 6) {
        text_line(output, "    mov [rsp + %d], %s", 8 * slot, SYSTEM_V_INTEGER[(*ints)++]);
    } else {
        text_line(output, "    mov rax, [rsp + .@frame + %d]", 8 + 8 * (*stack)++);
        text_line(output, "    mov [rsp + %d], rax", 8 * slot);
    }
}

static void emit_wrapper(TextBuffer *output, const Function *function) {
    text_line(output, "global %s:function", function->name);
    text_line(output, "$%s:", function->name);
    int slots = 0;
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        slots += parameter->symbol->length_label != NULL ? 2 : 1;
    }
    int reserved = slots < 4 ? 4 : slots;
    int frame = 8 * (reserved + (reserved % 2 == 0));
    text_line(output, ".@frame equ %d", frame);
    text_line(output, "    sub rsp, .@frame");
    int ints = 0, vectors = 0, stack = 0, slot = 0;
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        stage_argument(output, slot++, is_floating(parameter->type), &ints, &vectors, &stack);
        if (parameter->symbol->length_label != NULL) {
            stage_argument(output, slot++, false, &ints, &vectors, &stack);
        }
    }
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        int first = parameter->symbol->index;
        if (first >= 4) {
            break;
        }
        if (is_floating(parameter->type)) {
            text_line(output, "    movq xmm%d, [rsp + %d]", first, 8 * first);
        } else {
            text_line(output, "    mov %s, [rsp + %d]", RAZUM_INTEGER[first], 8 * first);
            if (parameter->symbol->length_label != NULL && first < 3) {
                text_line(output, "    mov %s, [rsp + %d]", RAZUM_INTEGER[first + 1], 8 * (first + 1));
            }
        }
    }
    text_line(output, "    call rzfn@%s", function->name);
    text_line(output, "    add rsp, .@frame");
    text_line(output, "    ret");
}

void emit_elf_object_header(TextBuffer *output, const Program *program) {
    text_line(output, "bits 64");
    text_line(output, "default rel");
    text_line(output, "pe@base equ 0");
    text_line(output, "section .note.GNU-stack noalloc noexec nowrite progbits");
    text_line(output, "section .text align=16");
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_exported) {
            emit_wrapper(output, function);
        }
    }
    for (int index = 0; index < program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_exported) {
            text_line(output, "global %s:data protected (%d)", variable->name, shape_size(variable->shape));
        }
    }
}
