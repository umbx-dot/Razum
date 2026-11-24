#include "checking/checking.h"

#include <stdint.h>

#include "common/arena.h"
#include "common/diagnostics.h"

static int64_t wrap(int64_t value, ValueType type) {
    if (type == TYPE_BOOL) {
        return value != 0;
    }
    const TypeInfo *info = type_info(type);
    if (info->size == 8) {
        return value;
    }
    int bits = 8 * info->size;
    uint64_t mask = ((uint64_t)1 << bits) - 1;
    uint64_t low = (uint64_t)value & mask;
    if (info->is_signed && (low >> (bits - 1)) != 0) {
        return (int64_t)(low | ~mask);
    }
    return (int64_t)low;
}

static Expression *make_int(const Expression *origin, int64_t value, ValueType type) {
    Expression *result = new_expression(EXPRESSION_INT, origin->line, origin->column);
    result->int_literal.value = wrap(value, type);
    result->int_literal.literal_type = type;
    result->int_literal.too_large = type == TYPE_ULONG && result->int_literal.value < 0;
    result->type = type;
    return result;
}

static Expression *make_double(const Expression *origin, double value, ValueType type) {
    Expression *result = new_expression(EXPRESSION_DOUBLE, origin->line, origin->column);
    result->double_literal = type == TYPE_FLOAT ? (double)(float)value : value;
    result->type = TYPE_DOUBLE;
    return result;
}

static double as_double(const Expression *value) {
    if (value->kind == EXPRESSION_DOUBLE) {
        return value->double_literal;
    }
    bool is_unsigned = is_integer(value->type) && !type_info(value->type)->is_signed;
    return is_unsigned ? (double)(uint64_t)value->int_literal.value : (double)value->int_literal.value;
}

static const double TWO_TO_63 = 9223372036854775808.0;

static int64_t as_integer(const Expression *value) {
    if (value->kind != EXPRESSION_DOUBLE) {
        return value->int_literal.value;
    }
    double number = value->double_literal;
    return number >= -TWO_TO_63 && number < TWO_TO_63 ? (int64_t)number : INT64_MIN;
}

static int64_t as_unsigned_long(const Expression *value) {
    double number = value->kind == EXPRESSION_DOUBLE ? value->double_literal : 0.0;
    if (value->kind != EXPRESSION_DOUBLE || number < TWO_TO_63) {
        return as_integer(value);
    }
    return number < 2.0 * TWO_TO_63 ? (int64_t)(uint64_t)number : 0;
}

static Expression *convert(const Expression *origin, const Expression *value, ValueType type) {
    if (is_floating(type)) {
        return make_double(origin, as_double(value), type);
    }
    if (type == TYPE_ULONG) {
        return make_int(origin, as_unsigned_long(value), type);
    }
    if (type == TYPE_BOOL || is_integer(type)) {
        return make_int(origin, as_integer(value), type);
    }
    return NULL;
}

static Expression *fold_floating(const Expression *expression, const Expression *left, const Expression *right) {
    double a = as_double(left);
    double b = as_double(right);
    switch (expression->binary.operator) {
        case OP_ADD: return make_double(expression, a + b, TYPE_DOUBLE);
        case OP_SUBTRACT: return make_double(expression, a - b, TYPE_DOUBLE);
        case OP_MULTIPLY: return make_double(expression, a * b, TYPE_DOUBLE);
        case OP_DIVIDE: return make_double(expression, a / b, TYPE_DOUBLE);
        case OP_EQUAL: return make_int(expression, a == b, TYPE_BOOL);
        case OP_NOT_EQUAL: return make_int(expression, a != b, TYPE_BOOL);
        case OP_LESS: return make_int(expression, a < b, TYPE_BOOL);
        case OP_LESS_EQUAL: return make_int(expression, a <= b, TYPE_BOOL);
        case OP_GREATER: return make_int(expression, a > b, TYPE_BOOL);
        case OP_GREATER_EQUAL: return make_int(expression, a >= b, TYPE_BOOL);
        default: return NULL;
    }
}

static Expression *fold_integer(const Expression *expression, const Expression *left, const Expression *right) {
    ValueType operation = expression->binary.operation;
    bool is_signed = operation == TYPE_BOOL || type_info(operation)->is_signed;
    int64_t a = wrap(left->int_literal.value, operation);
    int64_t b = wrap(right->int_literal.value, operation);
    uint64_t ua = (uint64_t)a;
    uint64_t ub = (uint64_t)b;
    Operator operator = expression->binary.operator;
    if ((operator == OP_DIVIDE || operator == OP_REMAINDER) && b == 0) {
        compile_error(right->line, right->column, "a constant divides by zero");
    }
    int shift = (int)(ub & (operation != TYPE_BOOL && type_info(operation)->size == 8 ? 63 : 31));
    switch (operator) {
        case OP_ADD: return make_int(expression, (int64_t)(ua + ub), operation);
        case OP_SUBTRACT: return make_int(expression, (int64_t)(ua - ub), operation);
        case OP_MULTIPLY: return make_int(expression, (int64_t)(ua * ub), operation);
        case OP_DIVIDE: return make_int(expression, is_signed ? (b == -1 ? (int64_t)(0 - ua) : a / b) : (int64_t)(ua / ub), operation);
        case OP_REMAINDER: return make_int(expression, is_signed ? (b == -1 ? 0 : a % b) : (int64_t)(ua % ub), operation);
        case OP_BIT_AND: return make_int(expression, a & b, operation);
        case OP_BIT_OR: return make_int(expression, a | b, operation);
        case OP_BIT_XOR: return make_int(expression, a ^ b, operation);
        case OP_SHIFT_LEFT: return make_int(expression, (int64_t)(ua << shift), operation);
        case OP_SHIFT_RIGHT: return make_int(expression, is_signed ? a >> shift : (int64_t)(ua >> shift), operation);
        case OP_EQUAL: return make_int(expression, a == b, TYPE_BOOL);
        case OP_NOT_EQUAL: return make_int(expression, a != b, TYPE_BOOL);
        case OP_LESS: return make_int(expression, is_signed ? a < b : ua < ub, TYPE_BOOL);
        case OP_LESS_EQUAL: return make_int(expression, is_signed ? a <= b : ua <= ub, TYPE_BOOL);
        case OP_GREATER: return make_int(expression, is_signed ? a > b : ua > ub, TYPE_BOOL);
        case OP_GREATER_EQUAL: return make_int(expression, is_signed ? a >= b : ua >= ub, TYPE_BOOL);
        default: return NULL;
    }
}

static bool is_number_literal(const Expression *value) {
    return value != NULL && value->kind != EXPRESSION_STRING;
}

static Expression *fold_logical(const Expression *expression) {
    Expression *left = fold_constant(expression->binary.left);
    if (!is_number_literal(left)) {
        return NULL;
    }
    bool a = as_integer(left) != 0;
    if (expression->binary.operator == OP_AND ? !a : a) {
        return make_int(expression, a, TYPE_BOOL);
    }
    Expression *right = fold_constant(expression->binary.right);
    return is_number_literal(right) ? make_int(expression, as_integer(right) != 0, TYPE_BOOL) : NULL;
}

static Expression *fold_binary(const Expression *expression) {
    Operator operator = expression->binary.operator;
    if (operator == OP_AND || operator == OP_OR) {
        return fold_logical(expression);
    }
    Expression *left = fold_constant(expression->binary.left);
    Expression *right = fold_constant(expression->binary.right);
    if (!is_number_literal(left) || !is_number_literal(right)) {
        return NULL;
    }
    if (is_floating(expression->binary.operation)) {
        return fold_floating(expression, left, right);
    }
    return fold_integer(expression, left, right);
}

static Expression *fold_unary(const Expression *expression) {
    Expression *operand = fold_constant(expression->unary.operand);
    if (!is_number_literal(operand)) {
        return NULL;
    }
    switch (expression->unary.operator) {
        case OP_NOT: return make_int(expression, as_integer(operand) == 0, TYPE_BOOL);
        case OP_BIT_NOT: return make_int(expression, ~operand->int_literal.value, expression->type);
        default:
            if (operand->kind == EXPRESSION_DOUBLE) {
                return make_double(expression, -operand->double_literal, TYPE_DOUBLE);
            }
            return make_int(expression, (int64_t)(0 - (uint64_t)operand->int_literal.value), expression->type);
    }
}

static Expression *fold_conditional(const Expression *expression) {
    Expression *condition = fold_constant(expression->conditional.condition);
    if (!is_number_literal(condition)) {
        return NULL;
    }
    const Expression *chosen = as_integer(condition) != 0 ? expression->conditional.when_true : expression->conditional.when_false;
    Expression *value = fold_constant(chosen);
    if (!is_number_literal(value)) {
        return value;
    }
    return convert(expression, value, expression->type);
}

Expression *fold_constant(const Expression *expression) {
    switch (expression->kind) {
        case EXPRESSION_INT:
        case EXPRESSION_DOUBLE:
        case EXPRESSION_STRING: {
            Expression *copy = arena_alloc(sizeof *copy);
            *copy = *expression;
            return copy;
        }
        case EXPRESSION_UNARY:
            return fold_unary(expression);
        case EXPRESSION_BINARY:
            return fold_binary(expression);
        case EXPRESSION_CAST: {
            Expression *operand = fold_constant(expression->cast.operand);
            return is_number_literal(operand) ? convert(expression, operand, expression->cast.target) : NULL;
        }
        case EXPRESSION_CONDITIONAL:
            return fold_conditional(expression);
        default:
            return NULL;
    }
}
