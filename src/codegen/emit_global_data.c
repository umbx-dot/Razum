#include "codegen/codegen.h"

#include <inttypes.h>
#include <string.h>

static void emit_padding(TextBuffer *output, int bytes) {
    if (bytes > 0) {
        text_line(output, "    times %d db 0", bytes);
    }
}

static void emit_scalar(TextBuffer *output, ProgramContext *program, ValueType type, const Expression *value) {
    if (value->kind == EXPRESSION_FUNCTION) {
        text_line(output, "%s dq pe@base + $%s", relocation_label(program), value->function_value->name);
        return;
    }
    if (type == TYPE_STRING) {
        text_line(output, "%s dq pe@base + str@%d", relocation_label(program), string_constant_index(program, value->string.bytes, value->string.length));
        return;
    }
    double real = value->kind == EXPRESSION_INT ? (double)value->int_literal.value : value->double_literal;
    if (type == TYPE_DOUBLE) {
        text_line(output, "    dq 0x%016" PRIX64 "    ; %.17g", double_bits(real), real);
        return;
    }
    if (type == TYPE_FLOAT) {
        float single = (float)real;
        uint32_t bits;
        memcpy(&bits, &single, sizeof bits);
        text_line(output, "    dd 0x%08" PRIX32 "    ; %.9g", bits, (double)single);
        return;
    }
    static const char *const DIRECTIVES[] = {"", "db", "dw", "", "dd", "", "", "", "dq"};
    text_line(output, "    %s %" PRIu64, DIRECTIVES[type_info(type)->size],
              (uint64_t)value->int_literal.value & (UINT64_MAX >> (64 - 8 * type_info(type)->size)));
}

static void emit_data(TextBuffer *output, ProgramContext *program, const Shape *shape, const Expression *value) {
    int size = shape_size(shape);
    if (value == NULL) {
        emit_padding(output, size);
        return;
    }
    if (!is_aggregate(shape->kind)) {
        emit_scalar(output, program, shape->kind, value);
        return;
    }
    if (value->kind == EXPRESSION_STRING) {
        text_append(output, "    db ");
        append_nasm_string(output, value->string.bytes, value->string.length);
        text_append(output, "\n");
        emit_padding(output, size - (int)value->string.length - 1);
        return;
    }
    if (shape->kind == TYPE_ARRAY) {
        for (int index = 0; index < shape->count; index++) {
            emit_data(output, program, shape->element, index < value->list.count ? LIST_AT(value->list, Expression, index) : NULL);
        }
        return;
    }
    int position = 0;
    for (int index = 0; index < shape->structure->fields.count; index++) {
        const Field *field = LIST_AT(shape->structure->fields, Field, index);
        emit_padding(output, field->offset - position);
        emit_data(output, program, field->shape, index < value->list.count ? LIST_AT(value->list, Expression, index) : NULL);
        position = field->offset + shape_size(field->shape);
    }
    emit_padding(output, size - position);
}

void emit_aggregate_global(TextBuffer *output, ProgramContext *program, const GlobalVariable *variable) {
    text_line(output, "    align 8, db 0");
    text_line(output, "%s:    ; %s", variable->symbol->label, describe_shape(variable->shape));
    emit_data(output, program, variable->shape, variable->initial);
}
