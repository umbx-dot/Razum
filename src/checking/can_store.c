#include "checking/checking.h"

bool literal_fits(const Expression *value, ValueType target) {
    if (value->kind == EXPRESSION_DOUBLE) {
        return target == TYPE_FLOAT;
    }
    if (value->kind != EXPRESSION_INT || !is_integer(target) || target == TYPE_CHAR) {
        return false;
    }
    ValueType literal = value->int_literal.literal_type;
    bool is_constant_integer = value->int_literal.from_constant && is_integer(literal) && literal != TYPE_CHAR;
    if (literal != TYPE_UNKNOWN && !is_constant_integer) {
        return false;
    }
    if (value->int_literal.too_large) {
        return target == TYPE_ULONG;
    }
    int64_t number = value->int_literal.value;
    const TypeInfo *info = type_info(target);
    if (info->size == 8) {
        return info->is_signed || number >= 0;
    }
    int bits = 8 * info->size;
    if (!info->is_signed) {
        return number >= 0 && number < ((int64_t)1 << bits);
    }
    return number >= -((int64_t)1 << (bits - 1)) && number < ((int64_t)1 << (bits - 1));
}

static bool is_char_array(const Expression *value) {
    return value->type == TYPE_ARRAY && value->shape->element->kind == TYPE_CHAR;
}

static bool points_to(const Shape *pointer, ValueType element) {
    return pointer != NULL && pointer->element != NULL && pointer->element->kind == element;
}

bool can_store_shape(const Shape *target, const Expression *value) {
    if (target->enumeration != NULL) {
        return value->type == TYPE_INT && value->shape != NULL && value->shape->enumeration == target->enumeration;
    }
    if (target->kind == TYPE_FUNCTION) {
        bool is_null = value->type == TYPE_POINTER && value->shape == NULL;
        return is_null || (value->type == TYPE_FUNCTION && same_shape(value->shape, target));
    }
    if (value->type == TYPE_FUNCTION) {
        return false;
    }
    if (target->kind == TYPE_POINTER) {
        if (value->type == TYPE_POINTER) {
            return value->shape == NULL || same_shape(value->shape->element, target->element);
        }
        if (value->type == TYPE_ARRAY) {
            return same_shape(value->shape->element, target->element);
        }
        return value->type == TYPE_STRING && target->element->kind == TYPE_CHAR;
    }
    if (target->kind == TYPE_STRING && value->type == TYPE_POINTER) {
        return points_to(value->shape, TYPE_CHAR);
    }
    if (value->type == TYPE_POINTER) {
        return false;
    }
    return can_store(target->kind, value);
}

bool can_store(ValueType target, const Expression *value) {
    if (is_aggregate(value->type)) {
        return target == TYPE_STRING && is_char_array(value);
    }
    return widens(value->type, target) || literal_fits(value, target);
}
