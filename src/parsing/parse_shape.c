#include "parsing/parsing.h"

#include "common/diagnostics.h"

enum { MAX_DIMENSIONS = 8 };

bool starts_declaration(const TokenStream *stream) {
    if (stream_is_type(stream, 0) || stream_is_keyword(stream, "fn")) {
        return true;
    }
    if (stream_peek(stream, 0)->kind != TOKEN_NAME) {
        return false;
    }
    int offset = 1;
    while (stream_is_operator(stream, "*", offset)) {
        offset++;
    }
    return stream_peek(stream, offset)->kind == TOKEN_NAME;
}

static Shape *parse_stars(TokenStream *stream, Shape *shape) {
    while (stream_is_operator(stream, "*", 0)) {
        const Token *star = stream_advance(stream);
        Shape *pointer = new_shape(TYPE_POINTER, star->line, star->column);
        pointer->element = shape;
        shape = pointer;
    }
    return shape;
}

static Shape *parse_function_shape(TokenStream *stream, const Token *start) {
    Shape *shape = new_shape(TYPE_FUNCTION, start->line, start->column);
    stream_expect_operator(stream, "(");
    if (!stream_is_operator(stream, ")", 0)) {
        do {
            Shape *parameter = parse_shape(stream);
            if (stream_current(stream)->kind == TOKEN_NAME) {
                stream_advance(stream);
            }
            list_push(&shape->parameters, parse_array_suffix(stream, parameter));
        } while (stream_accept_operator(stream, ","));
    }
    stream_expect_operator(stream, ")");
    shape->result = stream_accept_operator(stream, "->") ? parse_shape(stream) : new_shape(TYPE_VOID, start->line, start->column);
    return shape;
}

Shape *parse_shape(TokenStream *stream) {
    const Token *token = stream_current(stream);
    if (stream_accept_keyword(stream, "fn")) {
        return parse_function_shape(stream, token);
    }
    if (token->kind == TOKEN_NAME) {
        stream_advance(stream);
        Shape *shape = new_shape(TYPE_STRUCT, token->line, token->column);
        shape->struct_name = token->text;
        return parse_stars(stream, shape);
    }
    ValueType type = parse_type(stream);
    return parse_stars(stream, new_shape(type, token->line, token->column));
}

Shape *parse_array_suffix(TokenStream *stream, Shape *element) {
    int counts[MAX_DIMENSIONS];
    Expression *expressions[MAX_DIMENSIONS];
    int dimensions = 0;
    while (stream_is_operator(stream, "[", 0)) {
        const Token *open = stream_advance(stream);
        if (dimensions == MAX_DIMENSIONS) {
            compile_error(open->line, open->column, "an array has at most %d dimensions", MAX_DIMENSIONS);
        }
        counts[dimensions] = -1;
        expressions[dimensions] = NULL;
        const Token *size = stream_current(stream);
        if (size->kind == TOKEN_INT && stream_is_operator(stream, "]", 1)) {
            if (size->int_value == 0 || size->int_value > 0x7FFFFFFF) {
                compile_error(size->line, size->column, "an array needs between 1 and 2147483647 elements, not %s", size->text);
            }
            counts[dimensions] = (int)size->int_value;
            stream_advance(stream);
        } else if (!stream_is_operator(stream, "]", 0)) {
            expressions[dimensions] = parse_expression(stream);
            counts[dimensions] = 0;
        }
        stream_expect_operator(stream, "]");
        dimensions++;
    }
    Shape *shape = element;
    for (int index = dimensions - 1; index >= 0; index--) {
        Shape *array = new_shape(TYPE_ARRAY, element->line, element->column);
        array->element = shape;
        array->count = counts[index];
        array->count_expression = expressions[index];
        if (index > 0 && counts[index] < 0) {
            compile_error(element->line, element->column, "only the first size of an array may be left out, as in int grid[][4]");
        }
        shape = array;
    }
    return shape;
}
