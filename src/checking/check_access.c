#include "checking/checking.h"

#include <inttypes.h>
#include <string.h>

#include "common/diagnostics.h"

ValueType check_index(Checker *checker, Expression *expression) {
    Expression *base = expression->index.base;
    Expression *index = expression->index.index;
    ValueType base_type = check_any_expression(checker, base);
    if (base_type != TYPE_ARRAY && base_type != TYPE_STRING && (base_type != TYPE_POINTER || base->shape == NULL)) {
        compile_error(expression->line, expression->column, "[ ] needs an array, a pointer or a string, but this is %s", type_with_article(base->type));
    }
    ValueType index_type = check_expression(checker, index);
    if (!is_integer(index_type)) {
        compile_error(index->line, index->column, "an array index must be an integer, not %s", type_with_article(index_type));
    }
    if (base_type == TYPE_POINTER) {
        expression->shape = base->shape->element;
        return expression->shape->kind;
    }
    if (base_type == TYPE_STRING) {
        static const Shape CHARACTER = {.kind = TYPE_CHAR};
        expression->shape = &CHARACTER;
        return TYPE_CHAR;
    }
    int count = base->shape->count;
    if (index->kind == EXPRESSION_INT && count > 0 &&
        (index->int_literal.too_large || index->int_literal.value < 0 || index->int_literal.value >= count)) {
        compile_error(index->line, index->column, "index %" PRId64 " is outside an array of %d elements (0 to %d)",
                      index->int_literal.value, count, count - 1);
    }
    expression->shape = base->shape->element;
    return expression->shape->kind;
}

static ValueType enum_member(Expression *expression, const EnumType *enumeration) {
    const char *name = expression->field.name;
    for (int index = 0; index < enumeration->members.count; index++) {
        const EnumMember *member = LIST_AT(enumeration->members, EnumMember, index);
        if (strcmp(member->name, name) == 0) {
            expression->kind = EXPRESSION_INT;
            expression->int_literal.value = member->value;
            expression->int_literal.too_large = false;
            expression->int_literal.literal_type = TYPE_INT;
            expression->int_literal.from_constant = false;
            expression->shape = enumeration->shape;
            return TYPE_INT;
        }
    }
    compile_error(expression->line, expression->column, "enum '%s' has no member '%s'", enumeration->name, name);
}

ValueType check_field(Checker *checker, Expression *expression) {
    Expression *base = expression->field.base;
    const char *name = expression->field.name;
    if (base->kind == EXPRESSION_NAME && scopes_find(&checker->scopes, base->name.name) == NULL) {
        const EnumType *enumeration = find_enum(checker->program, base->name.name);
        if (enumeration != NULL) {
            return enum_member(expression, enumeration);
        }
    }
    ValueType base_type = check_any_expression(checker, base);
    if (base_type == TYPE_ARRAY && strcmp(name, "length") == 0) {
        expression->field.is_length = true;
        return TYPE_INT;
    }
    if (base_type == TYPE_ARRAY && rewrite_end_element(expression)) {
        return check_index(checker, expression);
    }
    bool through_pointer = base_type == TYPE_POINTER && base->shape != NULL && base->shape->element->kind == TYPE_STRUCT;
    if (base_type != TYPE_STRUCT && !through_pointer) {
        compile_error(expression->line, expression->column, "'.%s' needs a struct%s, but this is %s", name,
                      strcmp(name, "length") == 0 ? " or an array" : "", type_with_article(base_type));
    }
    const StructType *structure = through_pointer ? base->shape->element->structure : base->shape->structure;
    for (int index = 0; index < structure->fields.count; index++) {
        const Field *field = LIST_AT(structure->fields, Field, index);
        if (strcmp(field->name, name) == 0) {
            expression->field.field = field;
            expression->shape = field->shape;
            return field->shape->kind;
        }
    }
    compile_error(expression->line, expression->column, "struct '%s' has no field '%s'", structure->name, name);
}
