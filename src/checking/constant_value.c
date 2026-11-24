#include "checking/checking.h"

#include "common/arena.h"
#include "common/diagnostics.h"

static double number_of(const Expression *value) {
    if (value->kind == EXPRESSION_DOUBLE) {
        return value->double_literal;
    }
    return value->type == TYPE_ULONG ? (double)(uint64_t)value->int_literal.value : (double)value->int_literal.value;
}

const Expression *constant_value(Checker *checker, const Shape *shape, Expression *initial, const char *name) {
    ValueType kind = shape->kind;
    if (!is_numeric(kind) && kind != TYPE_BOOL && kind != TYPE_STRING) {
        compile_error(initial->line, initial->column, "a const must be a number, bool, char, string or enum, not %s", describe_shape(shape));
    }
    check_expression(checker, initial);
    Expression *value = fold_constant(initial);
    if (value == NULL) {
        compile_error(initial->line, initial->column,
                      "const '%s' needs a value known when compiling: literals, other consts and enum members, combined with operators", name);
    }
    if (value->kind == EXPRESSION_INT && is_integer(value->type) && value->type != TYPE_CHAR && value->shape == NULL) {
        value->int_literal.from_constant = true;
        value->int_literal.literal_type = value->type;
    }
    check_store(checker, shape, value, name);
    Expression *constant = arena_alloc(sizeof *constant);
    *constant = *value;
    if (is_floating(kind)) {
        double number = number_of(value);
        constant->kind = EXPRESSION_DOUBLE;
        constant->double_literal = kind == TYPE_FLOAT ? (double)(float)number : number;
    } else if (kind != TYPE_STRING) {
        constant->int_literal.literal_type = kind;
        constant->int_literal.from_constant = true;
        constant->int_literal.too_large = kind == TYPE_ULONG && constant->int_literal.value < 0;
    }
    constant->shape = shape;
    return constant;
}
