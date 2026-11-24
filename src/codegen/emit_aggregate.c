#include "codegen/codegen.h"

#include <stdio.h>

enum { UNROLL_LIMIT = 128 };

static void emit_zero(FunctionContext *context, const char *base, int size) {
    if (size <= UNROLL_LIMIT) {
        for (int offset = 0; offset < size; offset += 8) {
            emit(context, "mov qword [%s + %d], 0", base, offset);
        }
        return;
    }
    const char *loop = new_label(context, "zero");
    emit(context, "lea rax, [%s]", base);
    emit(context, "mov ecx, %d", size / 8);
    emit_label(context, loop);
    emit(context, "mov qword [rax], 0");
    emit(context, "add rax, 8");
    emit(context, "dec ecx");
    emit(context, "jnz %s", loop);
}

static void emit_fill_text(FunctionContext *context, const char *base, long long offset, const Expression *text) {
    size_t length = text->string.length;
    for (size_t index = 0; index < length; index += 4) {
        unsigned value = 0;
        size_t chunk = length - index < 4 ? length - index : 4;
        for (size_t byte = 0; byte < chunk; byte++) {
            value |= (unsigned)(unsigned char)text->string.bytes[index + byte] << (8 * byte);
        }
        const char *word = chunk == 4 ? "dword" : chunk >= 2 ? "word" : "byte";
        if (chunk == 3) {
            emit(context, "mov word [%s + %lld], 0x%X", base, offset + (long long)index, value & 0xFFFF);
            emit(context, "mov byte [%s + %lld], 0x%X", base, offset + (long long)index + 2, value >> 16);
        } else {
            emit(context, "mov %s [%s + %lld], 0x%X", word, base, offset + (long long)index, value);
        }
    }
}

static void emit_fill(FunctionContext *context, const char *base, long long offset, const Shape *shape, Expression *initial) {
    char place[OPERAND_SIZE];
    snprintf(place, sizeof place, "%s + %lld", base, offset);
    if (shape->kind == TYPE_ARRAY && initial->kind == EXPRESSION_STRING) {
        emit_fill_text(context, base, offset, initial);
        return;
    }
    if (initial->kind != EXPRESSION_LIST) {
        Operand operand = {0};
        if (is_floating(shape->kind)) {
            emit_double(context, initial);
            emit_store_to(context, shape->kind, place, false);
        } else if (int_operand(initial, &operand) && is_immediate(&operand)) {
            emit(context, "mov %s [%s], %s", size_word(shape->kind), place, operand.text);
        } else {
            emit_int(context, initial);
            emit_store_to(context, shape->kind, place, false);
        }
        return;
    }
    for (int index = 0; index < initial->list.count; index++) {
        Expression *item = LIST_AT(initial->list, Expression, index);
        if (shape->kind == TYPE_ARRAY) {
            emit_fill(context, base, offset + (long long)index * shape_size(shape->element), shape->element, item);
        } else {
            const Field *field = LIST_AT(shape->structure->fields, Field, index);
            emit_fill(context, base, offset + field->offset, field->shape, item);
        }
    }
}

static void emit_copy_bytes(FunctionContext *context, int size) {
    if (size > UNROLL_LIMIT) {
        const char *loop = new_label(context, "copy");
        emit(context, "mov r8d, %d", size / 8);
        emit_label(context, loop);
        emit(context, "mov rdx, qword [rcx]");
        emit(context, "mov qword [rax], rdx");
        emit(context, "add rcx, 8");
        emit(context, "add rax, 8");
        emit(context, "dec r8d");
        emit(context, "jnz %s", loop);
        size %= 8;
    }
    int offset = 0;
    static const struct { int width; const char *word; const char *scratch; } STEPS[] = {
        {8, "qword", "rdx"}, {4, "dword", "edx"}, {2, "word", "dx"}, {1, "byte", "dl"},
    };
    for (size_t step = 0; step < sizeof STEPS / sizeof STEPS[0]; step++) {
        while (size - offset >= STEPS[step].width) {
            emit(context, "mov %s, %s [rcx + %d]", STEPS[step].scratch, STEPS[step].word, offset);
            emit(context, "mov %s [rax + %d], %s", STEPS[step].word, offset, STEPS[step].scratch);
            offset += STEPS[step].width;
        }
    }
}

void emit_copy(FunctionContext *context, Expression *target, Expression *source) {
    char place[OPERAND_SIZE];
    int size = shape_size(target->shape);
    if (place_text(source, place, sizeof place)) {
        emit_address(context, target);
        emit(context, "lea rcx, [%s]", place);
    } else {
        Operand temp = acquire_temp(context);
        emit_address(context, source);
        emit(context, "mov %s, rax", temp.text);
        emit_address(context, target);
        emit(context, "mov rcx, %s", temp.text);
        release_temp(context);
    }
    emit_copy_bytes(context, size);
}

void emit_aggregate_declaration(FunctionContext *context, Statement *statement) {
    Symbol *symbol = statement->declaration.symbol;
    Expression *initial = statement->declaration.initial;
    Expression self = {0};
    self.kind = EXPRESSION_NAME;
    self.type = symbol->type;
    self.shape = symbol->shape;
    self.name.symbol = symbol;
    bool is_copy = initial != NULL && initial->kind != EXPRESSION_LIST && initial->kind != EXPRESSION_STRING;
    if (is_copy) {
        emit_copy(context, &self, initial);
        return;
    }
    char base[OPERAND_SIZE];
    snprintf(base, sizeof base, "rsp + %s", symbol->label);
    emit_zero(context, base, symbol->size);
    if (initial != NULL) {
        emit_fill(context, base, 0, symbol->shape, initial);
    }
}
