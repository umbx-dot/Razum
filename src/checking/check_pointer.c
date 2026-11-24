#include "checking/checking.h"

#include "common/diagnostics.h"

static Shape *pointer_to(const Shape *element, int line, int column) {
    Shape *pointer = new_shape(TYPE_POINTER, line, column);
    pointer->element = element;
    return pointer;
}

static bool is_place(const Expression *expression) {
    return expression->kind == EXPRESSION_NAME || expression->kind == EXPRESSION_INDEX || expression->kind == EXPRESSION_DEREF ||
           (expression->kind == EXPRESSION_FIELD && !expression->field.is_length);
}

ValueType check_address(Checker *checker, Expression *expression) {
    Expression *operand = expression->unary.operand;
    check_any_expression(checker, operand);
    if (operand->kind != EXPRESSION_FUNCTION && (!is_place(operand) || operand->type == TYPE_VOID)) {
        compile_error(expression->line, expression->column, "& needs a variable, element or field to point to");
    }
    if (operand->kind == EXPRESSION_FUNCTION) {
        expression->shape = operand->shape;
        return TYPE_FUNCTION;
    }
    if (operand->kind == EXPRESSION_NAME) {
        operand->name.symbol->address_taken = true;
    }
    expression->shape = pointer_to(operand->shape, expression->line, expression->column);
    return TYPE_POINTER;
}

ValueType check_deref(Checker *checker, Expression *expression) {
    Expression *operand = expression->unary.operand;
    check_expression(checker, operand);
    if (operand->type != TYPE_POINTER || operand->shape == NULL) {
        compile_error(expression->line, expression->column, "* needs a pointer, but this is %s", type_with_article(operand->type));
    }
    expression->shape = operand->shape->element;
    return expression->shape->kind;
}

ValueType check_new(Checker *checker, Expression *expression) {
    Shape *element = expression->allocation.element;
    resolve_shape(checker->program, element);
    if (element->kind == TYPE_VOID || (element->kind == TYPE_ARRAY && element->count < 0)) {
        compile_error(expression->line, expression->column, "new needs a type with a size, not %s", describe_shape(element));
    }
    Expression *count = expression->allocation.count;
    if (count != NULL && !is_integer(check_expression(checker, count))) {
        compile_error(count->line, count->column, "the number of elements must be an integer, not %s", type_with_article(count->type));
    }
    expression->shape = pointer_to(element, expression->line, expression->column);
    return TYPE_POINTER;
}

ValueType check_sizeof(Checker *checker, Expression *expression) {
    Shape *shape = expression->size_of;
    resolve_shape(checker->program, shape);
    if (shape->kind == TYPE_VOID || (shape->kind == TYPE_ARRAY && shape->count < 0)) {
        compile_error(expression->line, expression->column, "sizeof needs a type with a size, not %s", describe_shape(shape));
    }
    int size = shape_size(shape);
    expression->kind = EXPRESSION_INT;
    expression->int_literal.value = size;
    expression->int_literal.too_large = false;
    expression->int_literal.literal_type = TYPE_UNKNOWN;
    expression->shape = NULL;
    return TYPE_INT;
}

bool is_address_value(ValueType type) {
    return type == TYPE_POINTER || type == TYPE_FUNCTION;
}

bool pointers_compatible(const Expression *left, const Expression *right) {
    if (!is_address_value(left->type) || !is_address_value(right->type)) {
        return false;
    }
    if (left->shape == NULL || right->shape == NULL) {
        return true;
    }
    if (left->type != right->type) {
        return false;
    }
    return left->type == TYPE_FUNCTION ? same_shape(left->shape, right->shape) : same_shape(left->shape->element, right->shape->element);
}

Shape *function_shape(const Function *function, int line, int column) {
    Shape *shape = new_shape(TYPE_FUNCTION, line, column);
    shape->result = function->return_shape;
    for (int index = 0; index < function->parameters.count; index++) {
        const Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        if (parameter->type == TYPE_ARRAY && function->extern_dll != NULL) {
            compile_error(line, column, "'%s' is an extern function that takes an array, so it cannot be used as a function value; declare it with a pointer (%s *)",
                          function->name, describe_shape(parameter->shape->element));
        }
        list_push(&shape->parameters, parameter->shape);
    }
    return shape;
}
