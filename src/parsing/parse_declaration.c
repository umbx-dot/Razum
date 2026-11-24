#include "parsing/parsing.h"

Statement *parse_declaration(TokenStream *stream, bool require_semicolon) {
    const Token *start = stream_current(stream);
    Shape *base = parse_shape(stream);
    const Token *name = stream_expect_name(stream, "a variable name");
    Shape *shape = parse_array_suffix(stream, base);
    Statement *declaration = new_statement(STATEMENT_DECLARATION, start->line, start->column);
    declaration->declaration.type = shape->kind;
    declaration->declaration.shape = shape;
    declaration->declaration.name = name->text;
    if (stream_accept_operator(stream, "=")) {
        declaration->declaration.initial = parse_initializer(stream);
    }
    if (require_semicolon) {
        stream_expect_operator(stream, ";");
    }
    return declaration;
}
