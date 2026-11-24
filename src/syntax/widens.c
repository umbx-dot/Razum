#include "syntax/syntax.h"

bool is_integer(ValueType type) {
    return type_info(type)->kind == KIND_INTEGER;
}

bool is_floating(ValueType type) {
    return type_info(type)->kind == KIND_FLOATING;
}

bool is_numeric(ValueType type) {
    return is_integer(type) || is_floating(type);
}

bool widens(ValueType source, ValueType target) {
    if (source == target) {
        return true;
    }
    if (is_floating(target)) {
        return is_integer(source) || (source == TYPE_FLOAT && target == TYPE_DOUBLE);
    }
    if (!is_integer(source) || !is_integer(target) || target == TYPE_CHAR) {
        return false;
    }
    const TypeInfo *from = type_info(source);
    const TypeInfo *to = type_info(target);
    if (from->is_signed) {
        return to->is_signed && to->size >= from->size;
    }
    return to->is_signed ? to->size > from->size : to->size >= from->size;
}
