#include "checking/checking.h"

#include <inttypes.h>

#include "common/diagnostics.h"

static void report_literal_range(const Expression *value, ValueType target) {
    const TypeInfo *info = type_info(target);
    int bits = 8 * info->size;
    uint64_t maximum = info->is_signed ? (UINT64_MAX >> (65 - bits)) : (UINT64_MAX >> (64 - bits));
    int64_t minimum = info->is_signed ? -(int64_t)maximum - 1 : 0;
    if (value->int_literal.too_large) {
        compile_error(value->line, value->column, "%" PRIu64 " does not fit in %s, which holds %" PRId64 " to %" PRIu64,
                      (uint64_t)value->int_literal.value, info->name, minimum, maximum);
    }
    compile_error(value->line, value->column, "%" PRId64 " does not fit in %s, which holds %" PRId64 " to %" PRIu64,
                  value->int_literal.value, info->name, minimum, maximum);
}

static const char *describe_value(const Expression *value) {
    bool is_enum = value->shape != NULL && value->shape->enumeration != NULL;
    if (value->shape != NULL && (is_aggregate(value->type) || value->type == TYPE_POINTER || value->type == TYPE_FUNCTION || is_enum)) {
        return describe_shape(value->shape);
    }
    return type_with_article(value->type);
}

void check_store(Checker *checker, const Shape *target, Expression *value, const char *name) {
    check_expression(checker, value);
    store_text_as_char(target, value);
    if (can_store_shape(target, value)) {
        return;
    }
    ValueType kind = target->kind;
    bool is_plain_number = value->kind == EXPRESSION_INT && value->int_literal.literal_type == TYPE_UNKNOWN;
    if (is_plain_number && is_integer(kind) && kind != TYPE_CHAR && target->enumeration == NULL) {
        report_literal_range(value, kind);
    }
    compile_error(value->line, value->column, "cannot store %s in %s '%s'; convert it with (%s)",
                  describe_value(value), describe_shape(target), name, describe_shape(target));
}
