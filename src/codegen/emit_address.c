#include "codegen/codegen.h"

#include <stdio.h>

static bool is_constant_index(const Expression *index) {
    return index->kind == EXPRESSION_INT && !index->int_literal.too_large;
}

static bool static_offset(const Expression *expression, const Symbol **root, long long *offset) {
    switch (expression->kind) {
        case EXPRESSION_NAME:
            *root = expression->name.symbol;
            *offset = 0;
            return !(*root)->is_reference;
        case EXPRESSION_FIELD:
            if (expression->field.base->type != TYPE_STRUCT || !static_offset(expression->field.base, root, offset)) {
                return false;
            }
            *offset += expression->field.field->offset;
            return true;
        case EXPRESSION_INDEX:
            if (expression->index.base->type != TYPE_ARRAY || !is_constant_index(expression->index.index) ||
                !static_offset(expression->index.base, root, offset)) {
                return false;
            }
            *offset += expression->index.index->int_literal.value * shape_size(expression->shape);
            return true;
        default:
            return false;
    }
}

bool place_text(const Expression *expression, char *text, size_t size) {
    if (expression->kind == EXPRESSION_INDEX) {
        const Expression *base = expression->index.base;
        const Expression *index = expression->index.index;
        int scale = shape_size(expression->shape);
        if ((scale == 1 || scale == 2 || scale == 4 || scale == 8) &&
            (base->type == TYPE_POINTER || base->type == TYPE_STRING) && base->kind == EXPRESSION_NAME &&
            base->name.symbol->register_name != NULL && index->kind == EXPRESSION_NAME &&
            index->name.symbol->register_name != NULL) {
            snprintf(text, size, "%s + %s*%d", base->name.symbol->register_name, index->name.symbol->register_name, scale);
            return true;
        }
    }
    const Symbol *root = NULL;
    long long offset = 0;
    if (!static_offset(expression, &root, &offset)) {
        return false;
    }
    const char *base = root->kind == SYMBOL_GLOBAL ? "" : "rsp + ";
    if (offset == 0) {
        snprintf(text, size, "%s%s", base, root->label);
    } else {
        snprintf(text, size, "%s%s + %lld", base, root->label, offset);
    }
    return true;
}

static void emit_base_address(FunctionContext *context, Expression *base) {
    if (base->type == TYPE_STRING || base->type == TYPE_POINTER) {
        emit_int(context, base);
    } else {
        emit_address(context, base);
    }
}

static void emit_scaled_add(FunctionContext *context, int element_size) {
    if (element_size == 1 || element_size == 2 || element_size == 4 || element_size == 8) {
        emit(context, "lea rax, [rax + rcx*%d]", element_size);
        return;
    }
    emit(context, "imul rcx, rcx, %d", element_size);
    emit(context, "add rax, rcx");
}

static void emit_index_address(FunctionContext *context, Expression *expression) {
    Expression *base = expression->index.base;
    Expression *index = expression->index.index;
    int element_size = shape_size(expression->shape);
    if (is_constant_index(index)) {
        emit_base_address(context, base);
        if (index->int_literal.value != 0) {
            emit(context, "add rax, %lld", (long long)index->int_literal.value * element_size);
        }
        return;
    }
    char place[OPERAND_SIZE];
    bool is_simple_name = base->kind == EXPRESSION_NAME &&
                          (base->name.symbol->is_reference || base->type == TYPE_STRING || base->type == TYPE_POINTER);
    if (place_text(base, place, sizeof place) || is_simple_name) {
        emit_int(context, index);
        emit(context, "mov rcx, rax");
        emit_base_address(context, base);
    } else {
        emit_int(context, index);
        Operand temp = acquire_temp(context);
        emit(context, "mov %s, rax", temp.text);
        emit_base_address(context, base);
        emit(context, "mov rcx, %s", temp.text);
        release_temp(context);
    }
    emit_scaled_add(context, element_size);
}

void emit_address(FunctionContext *context, Expression *expression) {
    char place[OPERAND_SIZE];
    if (place_text(expression, place, sizeof place)) {
        emit(context, "lea rax, [%s]", place);
        return;
    }
    switch (expression->kind) {
        case EXPRESSION_NAME:
            emit(context, "mov rax, %s", memory_operand(expression->name.symbol).text);
            return;
        case EXPRESSION_DEREF:
            emit_int(context, expression->unary.operand);
            return;
        case EXPRESSION_FIELD:
            emit_base_address(context, expression->field.base);
            if (expression->field.field->offset != 0) {
                emit(context, "add rax, %d", expression->field.field->offset);
            }
            return;
        default:
            emit_index_address(context, expression);
            return;
    }
}

void emit_length(FunctionContext *context, const Expression *expression) {
    const Expression *array = expression->field.base;
    if (array->shape->count >= 0) {
        emit(context, "mov eax, %d", array->shape->count);
        return;
    }
    emit(context, "mov rax, qword [rsp + %s]", array->name.symbol->length_label);
}
