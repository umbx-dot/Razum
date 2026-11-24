#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

static const char *print_function_name(ValueType type, bool line_end) {
    switch (type) {
        case TYPE_BOOL: return line_end ? "print_bool" : "write_bool";
        case TYPE_CHAR: return line_end ? "print_char" : "write_char";
        case TYPE_ULONG: return line_end ? "print_ulong" : "write_ulong";
        case TYPE_STRING: return line_end ? "print_string" : "write_formatted_text";
        case TYPE_FLOAT: return line_end ? "print_float" : "write_float";
        case TYPE_DOUBLE: return line_end ? "print_double" : "write_double";
        default: return line_end ? "print_long" : "write_long";
    }
}

static ValueType check_print(Checker *checker, Expression *call, bool line_end) {
    const char *name = call->call.name;
    if (call->call.arguments.count != 1) {
        compile_error(call->line, call->column, "%s takes exactly one argument, not %d", name, call->call.arguments.count);
    }
    Expression *argument = LIST_AT(call->call.arguments, Expression, 0);
    ValueType type = check_expression(checker, argument);
    if (type == TYPE_ARRAY && can_store(TYPE_STRING, argument)) {
        type = TYPE_STRING;
    } else if (type == TYPE_POINTER && can_store(TYPE_STRING, argument)) {
        type = TYPE_STRING;
    } else if (is_aggregate(type)) {
        compile_error(argument->line, argument->column, "%s takes one value, not %s; print its elements or fields one by one",
                      name, describe_shape(argument->shape));
    } else if (type == TYPE_POINTER) {
        if (argument->shape != NULL && argument->shape->element->kind == TYPE_CHAR) {
            type = TYPE_STRING;
        } else {
            compile_error(argument->line, argument->column, "print a pointer's address as a number: %s((ulong)p)", name);
        }
    }
    Function *target = find_function(checker->program, print_function_name(type, line_end));
    if (target == NULL) {
        compile_error(call->line, call->column, "%s needs 'import stdio;' at the top of the file", name);
    }
    call->call.target = target;
    return TYPE_VOID;
}

static void check_aggregate_argument(const Expression *argument, const Parameter *parameter, const Function *function, int index) {
    const Shape *wanted = parameter->shape;
    const Shape *given = argument->shape;
    bool is_place = argument->kind == EXPRESSION_NAME || argument->kind == EXPRESSION_INDEX || argument->kind == EXPRESSION_FIELD;
    bool through_pointer = wanted->kind == TYPE_STRUCT && argument->type == TYPE_POINTER && given != NULL && same_shape(given->element, wanted);
    bool fits = through_pointer ||
                (is_place && is_aggregate(argument->type) &&
                 (wanted->kind == TYPE_ARRAY ? given->kind == TYPE_ARRAY && same_shape(given->element, wanted->element)
                                             : same_shape(given, wanted)));
    if (!fits) {
        compile_error(argument->line, argument->column, "argument %d of '%s' must be %s, not %s", index + 1, function->name,
                      describe_shape(wanted), is_aggregate(argument->type) ? describe_shape(given) : type_with_article(argument->type));
    }
}

static ValueType check_free(Checker *checker, Expression *call) {
    if (call->call.arguments.count != 1) {
        compile_error(call->line, call->column, "free takes exactly one pointer, not %d arguments", call->call.arguments.count);
    }
    Expression *argument = LIST_AT(call->call.arguments, Expression, 0);
    if (check_expression(checker, argument) != TYPE_POINTER) {
        compile_error(argument->line, argument->column, "free needs a pointer from new, not %s", type_with_article(argument->type));
    }
    call->call.is_free = true;
    return TYPE_VOID;
}

static ValueType check_indirect_call(Checker *checker, Expression *call) {
    Expression *callee = call->call.callee;
    if (check_expression(checker, callee) != TYPE_FUNCTION || callee->shape == NULL) {
        compile_error(call->line, call->column, "only functions can be called, but this is %s", type_with_article(callee->type));
    }
    const Shape *signature = callee->shape;
    if (call->call.arguments.count != signature->parameters.count) {
        compile_error(call->line, call->column, "%s takes %d argument(s), not %d", describe_shape(signature),
                      signature->parameters.count, call->call.arguments.count);
    }
    for (int index = 0; index < call->call.arguments.count; index++) {
        Expression *argument = LIST_AT(call->call.arguments, Expression, index);
        const Shape *parameter = LIST_AT(signature->parameters, Shape, index);
        check_expression(checker, argument);
        store_text_as_char(parameter, argument);
        bool through_pointer = parameter->kind == TYPE_STRUCT && argument->type == TYPE_POINTER && argument->shape != NULL &&
                               same_shape(argument->shape->element, parameter);
        bool by_reference = parameter->kind == TYPE_STRUCT && argument->type == TYPE_STRUCT && same_shape(argument->shape, parameter);
        bool is_place = argument->kind == EXPRESSION_NAME || argument->kind == EXPRESSION_INDEX || argument->kind == EXPRESSION_FIELD;
        bool by_array = parameter->kind == TYPE_ARRAY && is_place && argument->type == TYPE_ARRAY &&
                        same_shape(argument->shape->element, parameter->element);
        if (!through_pointer && !by_reference && !by_array && !can_store_shape(parameter, argument)) {
            compile_error(argument->line, argument->column, "argument %d must be %s, not %s", index + 1, describe_shape(parameter),
                          argument->shape != NULL && argument->type != TYPE_POINTER ? describe_shape(argument->shape) : type_with_article(argument->type));
        }
    }
    call->shape = signature->result;
    return signature->result->kind;
}

ValueType check_call(Checker *checker, Expression *call) {
    if (call->call.callee == NULL) {
        Symbol *variable = scopes_find(&checker->scopes, call->call.name);
        if (variable != NULL) {
            Expression *callee = new_expression(EXPRESSION_NAME, call->line, call->column);
            callee->name.name = call->call.name;
            call->call.callee = callee;
        }
    }
    if (call->call.callee != NULL && !rewrite_array_method(checker, call)) {
        return check_indirect_call(checker, call);
    }
    if (strcmp(call->call.name, "print") == 0) {
        return check_print(checker, call, true);
    }
    if (strcmp(call->call.name, "free") == 0) {
        return check_free(checker, call);
    }
    Function *function = find_function(checker->program, call->call.name);
    if (function == NULL) {
        if (strcmp(call->call.name, "printf") == 0) {
            return check_print(checker, call, false);
        }
        compile_error(call->line, call->column, "function '%s' is not declared", call->call.name);
    }
    if (call->call.arguments.count != function->parameters.count) {
        compile_error(call->line, call->column, "'%s' takes %d argument(s), not %d",
                      function->name, function->parameters.count, call->call.arguments.count);
    }
    for (int index = 0; index < call->call.arguments.count; index++) {
        Expression *argument = LIST_AT(call->call.arguments, Expression, index);
        Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        ValueType type = check_expression(checker, argument);
        store_text_as_char(parameter->shape, argument);
        if (is_aggregate(parameter->type)) {
            check_aggregate_argument(argument, parameter, function, index);
            continue;
        }
        if (!can_store_shape(parameter->shape, argument)) {
            compile_error(argument->line, argument->column, "argument %d of '%s' must be %s, not %s; convert it with (%s)",
                          index + 1, function->name, type_with_article(parameter->type), type_with_article(type), type_name(parameter->type));
        }
    }
    call->call.target = function;
    call->shape = function->return_shape;
    return function->return_type;
}
