#include "checking/checking.h"

#include <stdio.h>

#include "common/diagnostics.h"

static void require_numeric(const Expression *expression, const char *context) {
    if (!is_numeric(expression->type)) {
        compile_error(expression->line, expression->column, "%s needs a number, not %s", context, type_with_article(expression->type));
    }
}

void check_condition(Checker *checker, Expression *condition, const char *context) {
    ValueType type = check_expression(checker, condition);
    if (type != TYPE_BOOL && !is_address_value(type) && !is_integer(type)) {
        compile_error(condition->line, condition->column, "%s needs a bool or an integer, not %s; for a double write a comparison such as x != 0.0",
                      context, type_with_article(type));
    }
}

static void char_beside_integer(const Expression *other, Expression *text) {
    bool is_text = text->kind == EXPRESSION_STRING || (text->kind == EXPRESSION_CONDITIONAL && text->type == TYPE_STRING);
    if (is_text && (other->type == TYPE_CHAR || (is_integer(other->type) && is_one_character_text(text)))) {
        text_as_char(text);
    }
}

static ValueType check_binary(Checker *checker, Expression *expression) {
    Operator operator = expression->binary.operator;
    Expression *left = expression->binary.left;
    Expression *right = expression->binary.right;
    char context[8];
    snprintf(context, sizeof context, "'%s'", operator_text(operator));
    if (operator == OP_AND || operator == OP_OR) {
        check_condition(checker, left, context);
        check_condition(checker, right, context);
        return TYPE_BOOL;
    }
    bool is_equality = operator == OP_EQUAL || operator == OP_NOT_EQUAL;
    check_expression(checker, left);
    check_expression(checker, right);
    char_beside_integer(left, right);
    char_beside_integer(right, left);
    if (is_address_value(left->type) || is_address_value(right->type)) {
        if (!is_equality || !pointers_compatible(left, right)) {
            compile_error(expression->line, expression->column, "%s on pointers needs == or != and two pointers to the same type, or null", context);
        }
        expression->binary.operation = TYPE_ULONG;
        return TYPE_BOOL;
    }
    if (is_equality && left->type == TYPE_BOOL && right->type == TYPE_BOOL) {
        expression->binary.operation = TYPE_BOOL;
        return TYPE_BOOL;
    }
    bool is_bitwise = operator == OP_BIT_AND || operator == OP_BIT_OR || operator == OP_BIT_XOR;
    if (is_bitwise && left->type == TYPE_BOOL && right->type == TYPE_BOOL) {
        expression->binary.operation = TYPE_BOOL;
        return TYPE_BOOL;
    }
    require_numeric(left, context);
    require_numeric(right, context);
    bool needs_integers = operator == OP_REMAINDER || is_bitwise || operator == OP_SHIFT_LEFT || operator == OP_SHIFT_RIGHT;
    if (needs_integers && (!is_integer(left->type) || !is_integer(right->type))) {
        const Expression *wrong = is_integer(left->type) ? right : left;
        compile_error(wrong->line, wrong->column, "%s needs integers, not %s", context, type_with_article(wrong->type));
    }
    if (operator == OP_SHIFT_LEFT || operator == OP_SHIFT_RIGHT) {
        expression->binary.operation = promoted_type(left->type);
        return expression->binary.operation;
    }
    expression->binary.operation = operation_type(left, right, expression->line, expression->column, operator_text(operator));
    return operator >= OP_EQUAL && operator <= OP_GREATER_EQUAL ? TYPE_BOOL : expression->binary.operation;
}

static ValueType check_unary(Checker *checker, Expression *expression) {
    if (expression->unary.operator == OP_NOT) {
        check_condition(checker, expression->unary.operand, "'!'");
        return TYPE_BOOL;
    }
    ValueType type = check_expression(checker, expression->unary.operand);
    if (expression->unary.operator == OP_BIT_NOT) {
        if (!is_integer(type)) {
            compile_error(expression->line, expression->column, "'~' needs an integer, not %s", type_with_article(type));
        }
        return promoted_type(type);
    }
    require_numeric(expression->unary.operand, "'-'");
    return is_integer(type) && type_info(type)->size < 4 ? TYPE_INT : type;
}

static ValueType check_cast(Checker *checker, Expression *expression) {
    ValueType source = check_expression(checker, expression->cast.operand);
    resolve_shape(checker->program, expression->cast.shape);
    ValueType target = expression->cast.shape->kind;
    expression->cast.target = target;
    if (expression->cast.shape->enumeration != NULL) {
        expression->shape = expression->cast.shape;
    }
    if (target == TYPE_POINTER) {
        expression->shape = expression->cast.shape;
    }
    if (target == TYPE_FUNCTION) {
        expression->shape = expression->cast.shape;
    }
    bool is_address = is_address_value(source) || source == TYPE_STRING;
    bool allowed = (source == target && target != TYPE_FUNCTION) || (is_numeric(source) && is_numeric(target)) ||
                   (target == TYPE_FUNCTION && (is_address || is_integer(source))) ||
                   (source == TYPE_BOOL && is_integer(target)) || (is_integer(source) && target == TYPE_BOOL) ||
                   (target == TYPE_POINTER && (is_address || is_integer(source))) || (is_address && is_integer(target)) ||
                   (target == TYPE_STRING && source == TYPE_POINTER);
    if (!allowed) {
        compile_error(expression->line, expression->column, "%s cannot be converted to %s", type_with_article(source), type_name(target));
    }
    return target;
}

static ValueType check_conditional(Checker *checker, Expression *expression) {
    check_condition(checker, expression->conditional.condition, "a ? : condition");
    Expression *when_true = expression->conditional.when_true;
    Expression *when_false = expression->conditional.when_false;
    ValueType left = check_expression(checker, when_true);
    ValueType right = check_expression(checker, when_false);
    char_beside_integer(when_true, when_false);
    char_beside_integer(when_false, when_true);
    left = when_true->type;
    right = when_false->type;
    if (is_address_value(left) && is_address_value(right) && pointers_compatible(when_true, when_false)) {
        expression->shape = when_true->shape != NULL ? when_true->shape : when_false->shape;
        return left == TYPE_FUNCTION || right == TYPE_FUNCTION ? TYPE_FUNCTION : TYPE_POINTER;
    }
    if (left == right && !is_aggregate(left)) {
        bool same_enum = when_true->shape != NULL && when_false->shape != NULL && when_true->shape->enumeration != NULL &&
                         when_true->shape->enumeration == when_false->shape->enumeration;
        if (same_enum) {
            expression->shape = when_true->shape;
        }
        return left;
    }
    if (is_numeric(left) && is_numeric(right)) {
        return operation_type(when_true, when_false, expression->line, expression->column, "? :");
    }
    compile_error(expression->line, expression->column, "both sides of ? : must have one type, not %s and %s",
                  type_with_article(left), type_with_article(right));
}

static ValueType literal_type(const Expression *expression) {
    if (expression->int_literal.literal_type != TYPE_UNKNOWN) {
        return expression->int_literal.literal_type;
    }
    if (expression->int_literal.too_large) {
        return TYPE_ULONG;
    }
    int64_t value = expression->int_literal.value;
    return value >= INT32_MIN && value <= INT32_MAX ? TYPE_INT : TYPE_LONG;
}

ValueType check_any_expression(Checker *checker, Expression *expression) {
    switch (expression->kind) {
        case EXPRESSION_INT:
            expression->type = literal_type(expression);
            break;
        case EXPRESSION_DOUBLE:
            expression->type = TYPE_DOUBLE;
            break;
        case EXPRESSION_STRING:
            expression->type = TYPE_STRING;
            break;
        case EXPRESSION_NAME: {
            Symbol *symbol = scopes_find(&checker->scopes, expression->name.name);
            if (symbol != NULL && symbol->is_const) {
                if (symbol->constant == NULL) {
                    resolve_global_constant(checker->program, symbol->global);
                }
                int line = expression->line;
                int column = expression->column;
                *expression = *symbol->constant;
                expression->line = line;
                expression->column = column;
                if (symbol->type == TYPE_FLOAT) {
                    Expression *literal = new_expression(EXPRESSION_DOUBLE, line, column);
                    *literal = *expression;
                    expression->kind = EXPRESSION_CAST;
                    expression->cast.target = TYPE_FLOAT;
                    expression->cast.shape = (Shape *)symbol->shape;
                    expression->cast.operand = literal;
                }
                return check_any_expression(checker, expression);
            }
            Function *function = symbol == NULL ? find_function(checker->program, expression->name.name) : NULL;
            if (function != NULL) {
                expression->kind = EXPRESSION_FUNCTION;
                expression->function_value = function;
                function->address_taken = true;
                expression->shape = function_shape(function, expression->line, expression->column);
                expression->type = TYPE_FUNCTION;
                break;
            }
            expression->name.symbol = scopes_lookup(&checker->scopes, expression->name.name, expression->line, expression->column);
            expression->type = expression->name.symbol->type;
            expression->shape = expression->name.symbol->shape;
            break;
        }
        case EXPRESSION_FUNCTION:
            expression->type = TYPE_FUNCTION;
            break;
        case EXPRESSION_CALL:
            expression->type = check_call(checker, expression);
            break;
        case EXPRESSION_UNARY:
            expression->type = check_unary(checker, expression);
            break;
        case EXPRESSION_BINARY:
            expression->type = check_binary(checker, expression);
            break;
        case EXPRESSION_CAST:
            expression->type = check_cast(checker, expression);
            break;
        case EXPRESSION_INDEX:
            expression->type = check_index(checker, expression);
            break;
        case EXPRESSION_FIELD:
            expression->type = check_field(checker, expression);
            break;
        case EXPRESSION_CONDITIONAL:
            expression->type = check_conditional(checker, expression);
            break;
        case EXPRESSION_ADDRESS:
            expression->type = check_address(checker, expression);
            break;
        case EXPRESSION_DEREF:
            expression->type = check_deref(checker, expression);
            break;
        case EXPRESSION_NEW:
            expression->type = check_new(checker, expression);
            break;
        case EXPRESSION_SIZEOF:
            expression->type = check_sizeof(checker, expression);
            break;
        case EXPRESSION_LIST:
            compile_error(expression->line, expression->column, "a { } list can only give the first value of an array or struct");
    }
    return expression->type;
}

ValueType check_expression(Checker *checker, Expression *expression) {
    ValueType type = check_any_expression(checker, expression);
    if (type == TYPE_VOID) {
        compile_error(expression->line, expression->column, "'%s' is void, so it has no value to use", expression->call.name);
    }
    return type;
}
