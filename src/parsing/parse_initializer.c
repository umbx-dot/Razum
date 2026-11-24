#include "parsing/parsing.h"

Expression *parse_initializer(TokenStream *stream) {
    const Token *open = stream_current(stream);
    if (!stream_accept_operator(stream, "{")) {
        return parse_expression(stream);
    }
    Expression *list = new_expression(EXPRESSION_LIST, open->line, open->column);
    while (!stream_is_operator(stream, "}", 0)) {
        list_push(&list->list, parse_initializer(stream));
        if (!stream_accept_operator(stream, ",")) {
            break;
        }
    }
    stream_expect_operator(stream, "}");
    return list;
}
