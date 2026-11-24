#include "syntax/syntax.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"

Shape *new_shape(ValueType kind, int line, int column) {
    Shape *shape = arena_alloc(sizeof *shape);
    shape->kind = kind;
    shape->line = line;
    shape->column = column;
    return shape;
}

bool is_aggregate(ValueType type) {
    return type == TYPE_ARRAY || type == TYPE_STRUCT;
}

int shape_size(const Shape *shape) {
    switch (shape->kind) {
        case TYPE_ARRAY: return shape->count * shape_size(shape->element);
        case TYPE_STRUCT: return shape->structure->size;
        default: return type_info(shape->kind)->size;
    }
}

int shape_alignment(const Shape *shape) {
    switch (shape->kind) {
        case TYPE_ARRAY: return shape_alignment(shape->element);
        case TYPE_STRUCT: return shape->structure->alignment;
        default: return type_info(shape->kind)->size;
    }
}

static bool same_signature(const Shape *left, const Shape *right) {
    if (left->parameters.count != right->parameters.count || !same_shape(left->result, right->result)) {
        return false;
    }
    for (int index = 0; index < left->parameters.count; index++) {
        if (!same_shape(LIST_AT(left->parameters, Shape, index), LIST_AT(right->parameters, Shape, index))) {
            return false;
        }
    }
    return true;
}

bool same_shape(const Shape *left, const Shape *right) {
    if (left->kind != right->kind) {
        return false;
    }
    switch (left->kind) {
        case TYPE_ARRAY: return left->count == right->count && same_shape(left->element, right->element);
        case TYPE_STRUCT: return left->structure == right->structure;
        case TYPE_POINTER: return same_shape(left->element, right->element);
        case TYPE_FUNCTION: return same_signature(left, right);
        default: return left->enumeration == right->enumeration;
    }
}

static void describe_into(const Shape *shape, char *text, size_t size) {
    const Shape *base = shape;
    while (base->kind == TYPE_ARRAY) {
        base = base->element;
    }
    if (base->kind == TYPE_POINTER) {
        describe_into(base->element, text, size);
        size_t used = strlen(text);
        snprintf(text + used, size - used, "*");
    } else if (base->kind == TYPE_FUNCTION) {
        snprintf(text, size, "fn(");
        for (int index = 0; index < base->parameters.count; index++) {
            size_t used = strlen(text);
            snprintf(text + used, size - used, "%s", index > 0 ? ", " : "");
            used = strlen(text);
            describe_into(LIST_AT(base->parameters, Shape, index), text + used, size - used);
        }
        size_t used = strlen(text);
        snprintf(text + used, size - used, ")");
        if (base->result->kind != TYPE_VOID) {
            used = strlen(text);
            snprintf(text + used, size - used, " -> ");
            used = strlen(text);
            describe_into(base->result, text + used, size - used);
        }
    } else {
        const char *name = base->enumeration != NULL ? base->enumeration->name : type_name(base->kind);
        snprintf(text, size, "%s", base->kind == TYPE_STRUCT ? base->struct_name : name);
    }
    for (const Shape *level = shape; level->kind == TYPE_ARRAY; level = level->element) {
        size_t used = strlen(text);
        if (level->count < 0) {
            snprintf(text + used, size - used, "[]");
        } else {
            snprintf(text + used, size - used, "[%d]", level->count);
        }
    }
}

const char *describe_shape(const Shape *shape) {
    char text[256];
    describe_into(shape, text, sizeof text);
    return arena_copy_text(text, strlen(text));
}
