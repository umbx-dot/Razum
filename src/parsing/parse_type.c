#include "parsing/parsing.h"

#include "common/diagnostics.h"

ValueType parse_type(TokenStream *stream) {
    const Token *token = stream_current(stream);
    if (!stream_is_type(stream, 0)) {
        compile_error(token->line, token->column, "expected a type such as int, double, bool, char or string, found %s", describe_token(token));
    }
    stream_advance(stream);
    return type_from_name(token->text);
}
