#include "checking/checking.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"
#include "common/diagnostics.h"

static const char *const METHODS[] = {"sort", "sort_descending", "reverse", "fill", "contains", "index_of", "min", "max", "sum"};

static bool is_method(const char *name) {
    for (size_t index = 0; index < sizeof METHODS / sizeof METHODS[0]; index++) {
        if (strcmp(METHODS[index], name) == 0) {
            return true;
        }
    }
    return false;
}

static bool has_element_methods(ValueType element) {
    return is_numeric(element) || element == TYPE_CHAR;
}

bool rewrite_array_method(Checker *checker, Expression *call) {
    Expression *callee = call->call.callee;
    if (callee == NULL || callee->kind != EXPRESSION_FIELD || !is_method(callee->field.name)) {
        return false;
    }
    Expression *base = callee->field.base;
    if (check_any_expression(checker, base) != TYPE_ARRAY) {
        return false;
    }
    const Shape *element = base->shape->element;
    if (!has_element_methods(element->kind) || element->enumeration != NULL) {
        compile_error(callee->line, callee->column, ".%s() works on arrays of numbers or chars, not on %s", callee->field.name,
                      describe_shape(base->shape));
    }
    size_t size = strlen(callee->field.name) + strlen(type_name(element->kind)) + 8;
    char *name = arena_alloc(size);
    snprintf(name, size, "array_%s_%s", callee->field.name, type_name(element->kind));
    if (find_function(checker->program, name) == NULL) {
        compile_error(callee->line, callee->column, "values.%s() needs 'import array;' at the top of the file", callee->field.name);
    }
    PointerList arguments = {0};
    list_push(&arguments, base);
    for (int index = 0; index < call->call.arguments.count; index++) {
        list_push(&arguments, call->call.arguments.items[index]);
    }
    call->call.arguments = arguments;
    call->call.name = name;
    call->call.callee = NULL;
    return true;
}
