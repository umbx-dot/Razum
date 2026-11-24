#include "checking/checking.h"

#include "common/diagnostics.h"

static bool is_place(const Expression *expression) {
    return expression->kind == EXPRESSION_NAME || expression->kind == EXPRESSION_INDEX ||
           (expression->kind == EXPRESSION_FIELD && !expression->field.is_length);
}

void check_copy_source(Checker *checker, const Shape *target, Expression *value, const char *name) {
    check_any_expression(checker, value);
    if (!is_place(value) || !is_aggregate(value->type) || !same_shape(value->shape, target)) {
        compile_error(value->line, value->column, "%s '%s' can only be set from another %s or a { } list, not from %s",
                      describe_shape(target), name, describe_shape(target),
                      is_aggregate(value->type) ? describe_shape(value->shape) : type_with_article(value->type));
    }
}

static void check_list(Checker *checker, Shape *shape, Expression *list, const char *name) {
    if (shape->kind == TYPE_ARRAY) {
        if (shape->count < 0) {
            shape->count = list->list.count;
        }
        if (list->list.count > shape->count) {
            compile_error(list->line, list->column, "%d values do not fit in %s '%s'", list->list.count, describe_shape(shape), name);
        }
        for (int index = 0; index < list->list.count; index++) {
            check_initial_value(checker, (Shape *)shape->element, LIST_AT(list->list, Expression, index), name);
        }
        return;
    }
    const StructType *structure = shape->structure;
    if (list->list.count > structure->fields.count) {
        compile_error(list->line, list->column, "struct '%s' has %d fields, not %d", structure->name, structure->fields.count, list->list.count);
    }
    for (int index = 0; index < list->list.count; index++) {
        Field *field = LIST_AT(structure->fields, Field, index);
        check_initial_value(checker, field->shape, LIST_AT(list->list, Expression, index), field->name);
    }
}

void check_initial_value(Checker *checker, Shape *shape, Expression *initial, const char *name) {
    if (!is_aggregate(shape->kind)) {
        if (initial->kind == EXPRESSION_LIST) {
            compile_error(initial->line, initial->column, "a { } list needs an array or struct, but '%s' is %s", name, type_with_article(shape->kind));
        }
        check_store(checker, shape, initial, name);
        return;
    }
    bool is_text = shape->kind == TYPE_ARRAY && shape->element->kind == TYPE_CHAR && initial->kind == EXPRESSION_STRING;
    if (is_text) {
        int needed = (int)initial->string.length + 1;
        if (shape->count < 0) {
            shape->count = needed;
        }
        if (needed > shape->count) {
            compile_error(initial->line, initial->column, "the text needs %d chars with its closing \\0, but '%s' holds %d", needed, name, shape->count);
        }
        initial->type = TYPE_STRING;
        return;
    }
    if (initial->kind == EXPRESSION_LIST) {
        check_list(checker, shape, initial, name);
        initial->type = shape->kind;
        initial->shape = shape;
        return;
    }
    if (shape->kind == TYPE_ARRAY && shape->count < 0) {
        compile_error(initial->line, initial->column, "'%s' has no size, so it needs a { } list or text to count", name);
    }
    check_copy_source(checker, shape, initial, name);
}
