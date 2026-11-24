#include "checking/checking.h"

#include <inttypes.h>

#include "common/diagnostics.h"

void resolve_count(Checker *checker, Shape *shape) {
    Expression *count = shape->count_expression;
    if (count == NULL) {
        return;
    }
    shape->count_expression = NULL;
    check_expression(checker, count);
    const Expression *value = fold_constant(count);
    if (value == NULL || value->kind != EXPRESSION_INT || !is_integer(value->type)) {
        compile_error(count->line, count->column, "an array size must be a whole number known when compiling, such as 10 or SIZE * 2");
    }
    if (value->int_literal.too_large || value->int_literal.value < 1 || value->int_literal.value > 0x7FFFFFFF) {
        compile_error(count->line, count->column, "an array needs between 1 and 2147483647 elements, not %" PRId64, value->int_literal.value);
    }
    shape->count = (int)value->int_literal.value;
}
