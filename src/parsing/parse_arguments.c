#include "parsing/parsing.h"

PointerList parse_arguments(TokenStream *stream) {
    PointerList arguments = {0};
    stream_expect_operator(stream, "(");
    if (!stream_is_operator(stream, ")", 0)) {
        do {
            list_push(&arguments, parse_expression(stream));
        } while (stream_accept_operator(stream, ","));
    }
    stream_expect_operator(stream, ")");
    return arguments;
}
