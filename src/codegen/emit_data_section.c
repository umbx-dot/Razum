#include "codegen/codegen.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static void emit_global(TextBuffer *output, ProgramContext *program, const GlobalVariable *variable) {
    const Expression *initial = variable->initial;
    if (initial == NULL && variable->type != TYPE_STRING) {
        text_line(output, "%s: dq 0", variable->symbol->label);
        return;
    }
    if (initial != NULL && initial->kind == EXPRESSION_FUNCTION) {
        text_line(output, "%s:", variable->symbol->label);
        text_line(output, "%s dq pe@base + $%s", relocation_label(program), initial->function_value->name);
        return;
    }
    if (variable->type == TYPE_STRING) {
        text_line(output, "%s:", variable->symbol->label);
        text_line(output, "%s dq pe@base + str@%d", relocation_label(program), variable->string_index);
        return;
    }
    double real = initial->kind == EXPRESSION_INT ? (double)initial->int_literal.value : initial->double_literal;
    if (variable->type == TYPE_DOUBLE) {
        text_line(output, "%s: dq 0x%016" PRIX64 "    ; %.17g", variable->symbol->label, double_bits(real), real);
        return;
    }
    if (variable->type == TYPE_FLOAT) {
        float single = (float)real;
        uint32_t bits;
        memcpy(&bits, &single, sizeof bits);
        text_line(output, "%s: dq 0x%08" PRIX32 "    ; %.9g", variable->symbol->label, bits, (double)single);
        return;
    }
    if (variable->type == TYPE_ULONG) {
        text_line(output, "%s: dq %" PRIu64, variable->symbol->label, (uint64_t)initial->int_literal.value);
        return;
    }
    text_line(output, "%s: dq %" PRId64, variable->symbol->label, initial->int_literal.value);
}

static void emit_string(TextBuffer *output, int index, const StringConstant *constant) {
    char label[32];
    snprintf(label, sizeof label, "str@%d: db ", index);
    text_append(output, label);
    append_nasm_string(output, constant->bytes, constant->length);
    text_append(output, "\n");
}

void emit_data_section(TextBuffer *output, ProgramContext *program, const Program *source_program) {
    text_line(output, "");
    text_line(output, "    align 8, db 0");
    for (int index = 0; index < program->numbers.count; index++) {
        uint64_t bits = *LIST_AT(program->numbers, uint64_t, index);
        double value;
        memcpy(&value, &bits, sizeof value);
        text_line(output, "num@%d: dq 0x%016" PRIX64 "    ; %.17g", index + 1, bits, value);
    }
    for (int index = 0; index < source_program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(source_program->globals, GlobalVariable, index);
        if (variable->is_library && !variable->symbol->is_used) {
            continue;
        }
        if (is_aggregate(variable->type)) {
            emit_aggregate_global(output, program, variable);
        } else {
            emit_global(output, program, variable);
        }
    }
    for (int index = 0; index < program->strings.count; index++) {
        emit_string(output, index + 1, LIST_AT(program->strings, StringConstant, index));
    }
}
