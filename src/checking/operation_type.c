#include "checking/checking.h"

#include "common/diagnostics.h"

ValueType promoted_type(ValueType type) {
    return is_integer(type) && type_info(type)->size < 4 ? TYPE_INT : type;
}

ValueType operation_type(const Expression *left, const Expression *right, int line, int column, const char *operator) {
    ValueType left_type = left->type;
    ValueType right_type = right->type;
    if (is_floating(left_type) || is_floating(right_type)) {
        return left_type == TYPE_DOUBLE || right_type == TYPE_DOUBLE ? TYPE_DOUBLE : TYPE_FLOAT;
    }
    left_type = promoted_type(left_type);
    right_type = promoted_type(right_type);
    if (literal_fits(left, right_type)) {
        left_type = right_type;
    } else if (literal_fits(right, left_type)) {
        right_type = left_type;
    }
    if (left_type == right_type) {
        return left_type;
    }
    const TypeInfo *left_info = type_info(left_type);
    const TypeInfo *right_info = type_info(right_type);
    if (left_info->is_signed == right_info->is_signed) {
        return left_info->size >= right_info->size ? left_type : right_type;
    }
    ValueType signed_type = left_info->is_signed ? left_type : right_type;
    ValueType unsigned_type = left_info->is_signed ? right_type : left_type;
    if (type_info(signed_type)->size > type_info(unsigned_type)->size) {
        return signed_type;
    }
    if (type_info(unsigned_type)->size == 8) {
        compile_error(line, column, "'%s' mixes %s and %s; convert one side with (long) or (ulong)",
                      operator, type_with_article(left->type), type_with_article(right->type));
    }
    return TYPE_LONG;
}
