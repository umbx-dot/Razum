#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

bool rewrite_end_element(Expression *expression) {
    const char *name = expression->field.name;
    bool is_first = strcmp(name, "first") == 0;
    if (!is_first && strcmp(name, "last") != 0) {
        return false;
    }
    Expression *base = expression->field.base;
    bool is_simple = base->kind == EXPRESSION_NAME || (base->kind == EXPRESSION_FIELD && base->field.base->kind == EXPRESSION_NAME);
    if (!is_simple) {
        compile_error(expression->line, expression->column, "use .%s on a variable or a field, such as values.%s; store this array in a variable first",
                      name, name);
    }
    Expression *index = new_expression(EXPRESSION_INT, expression->line, expression->column);
    if (!is_first) {
        Expression *length = new_expression(EXPRESSION_FIELD, expression->line, expression->column);
        length->field.base = base;
        length->field.name = "length";
        Expression *one = new_expression(EXPRESSION_INT, expression->line, expression->column);
        one->int_literal.value = 1;
        Expression *last = new_expression(EXPRESSION_BINARY, expression->line, expression->column);
        last->binary.operator = OP_SUBTRACT;
        last->binary.left = length;
        last->binary.right = one;
        index = last;
    }
    expression->kind = EXPRESSION_INDEX;
    expression->index.base = base;
    expression->index.index = index;
    return true;
}
